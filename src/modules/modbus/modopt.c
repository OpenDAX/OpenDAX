/* Dax Modbus - Modbus (tm) Communications Module
 * Copyright (C) 2006 Phil Birkelbach
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Source file containing the configuration options
 */

#define _GNU_SOURCE
#include <common.h>
#include "modopt.h"
#include <opendax.h>
#include <libdaxlua.h>


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <getopt.h>
#include <termios.h>


static void printconfig(void);

struct Config config;
extern dax_state *ds;

static void
_init_config(void)
{
    config.portcount = 0;
    config.portsize = 1;
    config.threads = NULL;
    config.ports = NULL;
}

static inline int
_get_serial_config(lua_State *L, mb_port *p)
{
    char *string;
    char *device;
    int baudrate;
    short databits;
    short stopbits;
    short parity;
    int result;

    if(!lua_istable(L, -1)) {
        luaL_error(L, "_get_serial_config() the top of the Lua stack is not a table");
    }

    lua_getfield(L, -1, "device");
    device = (char *)lua_tostring(L, -1);
    if(device == NULL) {
        dax_log(LOG_WARN, "No device given for serial port %s, Using /dev/serial", p->name);
        device = strdup("/dev/serial");
    }

    lua_getfield(L, -2, "baudrate");
    baudrate = (int)lua_tonumber(L, -1);
    if(baudrate == 0) {
        dax_log(LOG_WARN, "Unknown Baudrate, Using 9600");
        baudrate = 9600;
    }

    lua_getfield(L, -3, "databits");
    databits = (short)lua_tonumber(L, -1);
    if(databits < 7 || databits > 8) {
        dax_log(LOG_WARN, "Unknown databits - %d, Using 8", databits);
        databits = 8;
    }

    lua_getfield(L, -4, "stopbits");
    stopbits = (unsigned int)lua_tonumber(L, -1);
    if(stopbits != 1 && stopbits != 2) {
        dax_log(LOG_WARN, "Unknown stopbits - %d, Using 1", stopbits);
        stopbits = 1;
    }

    lua_getfield(L, -5, "parity");
    if(lua_isnumber(L, -1)) {
        parity = (unsigned char)lua_tonumber(L, -1);
        if(parity != MB_ODD && parity != MB_EVEN && parity != MB_NONE) {
            dax_log(LOG_WARN, "Unknown Parity %d, using NONE", parity);
        }
    } else {
        string = (char *)lua_tostring(L, -1);
        if(string) {
            if(strcasecmp(string, "NONE") == 0) parity = MB_NONE;
            else if(strcasecmp(string, "EVEN") == 0) parity = MB_EVEN;
            else if(strcasecmp(string, "ODD") == 0) parity = MB_ODD;
            else {
                dax_log(LOG_WARN, "Unknown Parity %s, using NONE", string);
                parity = MB_NONE;
            }
        } else {
            dax_log(LOG_WARN, "Parity not given, using NONE");
            parity = MB_NONE;
        }
    }
    result = mb_set_serial_port(p, device, baudrate, databits, parity, stopbits);
    lua_pop(L, 5);
    return result;
}

static inline int
_get_network_config(lua_State *L, mb_port *p)
{
    char *ipaddress;
    char *string;
    unsigned int bindport;    /* IP port to bind to */
    unsigned char socket;     /* either UDP_SOCK or TCP_SOCK */
    int result;

    lua_getfield(L, -1, "ipaddress");
    ipaddress = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "bindport");
    bindport = (unsigned int)lua_tonumber(L, -1);
    if(bindport == 0) bindport = 502;
    lua_pop(L, 1);

    lua_getfield(L, -1, "socket");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "TCP") == 0) socket = TCP_SOCK;
        else if(strcasecmp(string, "UDP") == 0) socket = UDP_SOCK;
        else {
            dax_log(LOG_WARN, "Unknown Socket Type %s, using TCP", string);
            socket = TCP_SOCK;
        }
    } else {
        dax_log(LOG_WARN, "Socket Type not given, using TCP");
        socket = TCP_SOCK;
    }
    lua_pop(L, 1);
    if(ipaddress != NULL) {
        result = mb_set_network_port(p, ipaddress, bindport, socket);
    } else {
        result = mb_set_network_port(p, "0.0.0.0", bindport, socket);
    }
    return result;
}


/* Lua interface function for adding a port.  It takes a single
   table as an argument and returns the Port's index */
static int
_add_port(lua_State *L)
{
    mb_port *p;
    mb_port **newports;
    char *string, *name;
    int tmp, maxfailures, inhibit;
    unsigned char devtype, protocol, type;

    if(!lua_istable(L, -1)) {
        luaL_error(L, "add_port() received an argument that is not a table");
    }

    /* This logic allocates the port array if it does not already exist */
    if(config.ports == NULL) {
        config.ports = malloc(sizeof(mb_port *) * DEFAULT_PORTS);
        config.portsize = DEFAULT_PORTS;
        if(config.ports == NULL) {
            dax_log(LOG_FATAL, "Unable to allocate port array");
            kill(getpid(), SIGQUIT);
        }
    }
    /* Check to makes sure that we have some ports left */
    if(config.portcount >= config.portsize) {
        /* Double the size of the array */
        newports = realloc(config.ports, sizeof(mb_port *) * config.portsize * 2);
        if(newports != NULL) {
            config.ports = newports;
            config.portsize *= 2;
        } else {
            dax_log(LOG_FATAL, "Unable to reallocate port array");
            kill(getpid(), SIGQUIT);
        }
    }

    dax_log(LOG_MINOR, "Adding a port at index = %d", config.portcount);

    lua_getfield(L, -1, "name");
    name = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    config.ports[config.portcount] = mb_new_port(name, 0x0000);
    /* Assign the pointer to p to make things simpler */
    p = config.ports[config.portcount];
    if(p == NULL) {
        dax_log(LOG_FATAL, "Unable to allocate port[%d]", config.portcount);
        kill(getpid(), SIGQUIT);
    }
    config.portcount++;

    lua_getfield(L, -1, "enable");
    p->enable = (char)lua_toboolean(L, -1);
    lua_pop(L, 1);


    /* What Modbus protocol are we going to talk on this port */
    lua_getfield(L, -1, "protocol");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "RTU") == 0) {
            protocol = MB_RTU;
            devtype = SERIAL_PORT;
        } else if(strcasecmp(string, "ASCII") == 0) {
            protocol = MB_ASCII;
            devtype = SERIAL_PORT;
        } else if(strcasecmp(string, "TCP") == 0) {
            protocol = MB_TCP;
            devtype = NETWORK_PORT;
        } else {
            dax_log(LOG_WARN, "Unknown Protocol %s, assuming RTU", string);
            protocol = MB_RTU;
            devtype = SERIAL_PORT;
        }
    } else {
        dax_log(LOG_WARN, "Protocol not given, assuming RTU");
        protocol = MB_RTU;
        devtype = SERIAL_PORT;
    }
    lua_pop(L,1);
    /* Serial port and network configurations need different data.  These
       two functions will get the right stuff out of the table.  The table
       is not checked to see if too much information is given only that
       enough information is given to do the job */
    if(devtype == SERIAL_PORT) {
        _get_serial_config(L, p);
    } else if(devtype == NETWORK_PORT) {
        _get_network_config(L, p);
    }


    lua_getfield(L, -1, "type");
    string = (char *)lua_tostring(L, -1);
    if(strcasecmp(string, "MASTER") == 0) type = MB_MASTER;
    else if(strcasecmp(string, "SLAVE") == 0) type = MB_SLAVE;
    else if(strcasecmp(string, "CLIENT") == 0) type = MB_MASTER;
    else if(strcasecmp(string, "SERVER") == 0) type = MB_SLAVE;
    else {
        dax_log(LOG_WARN, "Unknown Port Type %s, assuming MASTER", string);
        type = MB_MASTER;
    }
    lua_pop(L, 1);

    mb_set_protocol(p, type, protocol);

    /* Have to decide how much of this will really be needed */
    lua_getfield(L, -1, "delay");
    p->delay = (unsigned int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "frame");
    tmp = (unsigned int)lua_tonumber(L, -1);
    if(tmp > 0) p->frame = tmp;
    lua_pop(L, 1);

    lua_getfield(L, -1, "scanrate");
    p->scanrate = (unsigned int)lua_tonumber(L, -1);
    if(p->scanrate == 0) p->scanrate = 100;
    lua_pop(L, 1);

    lua_getfield(L, -1, "timeout");
    p->timeout = (unsigned int)lua_tonumber(L, -1);
    if(p->timeout == 0) p->timeout = 1000;
    lua_pop(L, 1);

    lua_getfield(L, -1, "retries");
    p->retries = (unsigned int)lua_tonumber(L, -1);
    if(p->retries == 0) p->timeout = 1;
    if(p->retries > MAX_RETRIES) p->retries = MAX_RETRIES;
    lua_pop(L, 1);

    lua_getfield(L, -1, "inhibit");
    inhibit = (unsigned int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "maxfailures");
    maxfailures = (unsigned int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    mb_set_maxfailures(p, maxfailures, inhibit);

    lua_getfield(L, -1, "persist");
    if(lua_toboolean(L, -1)) {
        p->persist = 1;
    } else {
        p->persist = 0;
    }
    lua_pop(L, 1);

    if(p->type == MB_SLAVE) {
        p->nodes = malloc(sizeof(mb_node_def) * MB_MAX_SLAVE_NODES);
        if(p->nodes == NULL) {
            dax_log(LOG_FATAL, "Unable to allocate node definitions for port[%d]", config.portcount);
            kill(getpid(), SIGQUIT);
        }
        bzero(p->nodes, sizeof(mb_node_def) * MB_MAX_SLAVE_NODES);
    }

    /* The lua script gets the index +1 */
    lua_pushnumber(L, config.portcount);

    return 1;
}


/* Lua interface function for adding a modbus command to a port.
   Accepts two arguments.  The first is the port to assign the command
   too, and the second is a table describing the command. */
static int
_add_command(lua_State *L)
{
    mb_cmd *c;
    const char *string;
    int result;
    uint8_t node, function;
    uint16_t reg, length;
    int p; /* Port ID */
    mb_port *port;

    p = (int)lua_tonumber(L, 1);
    p--; /* Lua has indexes that are 1+ our actual array indexes */
    if(p < 0 || p >= config.portcount) {
        luaL_error(L, "Unknown Port ID : %d", p+1);
    }
    if(!lua_istable(L, 2)) {
        luaL_error(L, "add_command() received an argument that is not a table");
    }
    port = config.ports[p];
    if(port->type != MB_MASTER) {
        dax_log(LOG_WARN, "Adding commands only makes sense for a Master or Client port");
        return 0;
    }
    dax_log(LOG_DEBUG, "Adding a command to port %d", p);

    /* Allocate the new command and add it to the port */
    c = mb_new_cmd(config.ports[p]);
    if(c == NULL) {
        luaL_error(L, "Can't allocate memory for the command");
    }

    lua_getfield(L, -1, "node");
    node = (uint8_t)lua_tonumber(L, -1);

    lua_getfield(L, -2, "fcode");
    function = (uint8_t)lua_tonumber(L, -1);

    lua_getfield(L, -3, "register");
    reg = (uint16_t)lua_tonumber(L, -1);

    lua_getfield(L, -4, "length");
    length = (uint16_t)lua_tonumber(L, -1);

    if(mb_set_command(c, node, function, reg, length)) {
        dax_log(LOG_ERROR, "Unable to set command");
    }
    lua_pop(L, 4);

    /* mode is how the command will be sent...
     *    CONTINUOUS = sent periodically at each "interval" of the ports scanrate
     *    TRIGGER = is sent when the trigger tag is set
     *    CHANGE = sent when the data tag has changed
     *    WRITE = sent when another client module writes to the data tag regarless of chagne
     */
    lua_getfield(L, -1, "mode");
    string = (char *)lua_tostring(L, -1);
    if(string != NULL) {
        if(strcasestr(string, "CONT") != NULL) c->mode = MB_CONTINUOUS;
        if(strcasestr(string, "TRIGGER") != NULL) c->mode |= MB_TRIGGER;
        if(strcasestr(string, "CHANGE") != NULL) {
            if(mb_is_write_cmd(c)) {
                c->mode |= MB_ONCHANGE;
            } else {
                dax_log(LOG_ERROR, "ON CHANGE command trigger only makes sense for write commands");
            }
        }
        if(strcasestr(string, "WRITE") != NULL) {
            if(mb_is_write_cmd(c)) {
                c->mode |= MB_ONWRITE;
            } else {
                dax_log(LOG_ERROR, "ON WRITE command trigger only makes sense for write commands");
            }
        }
    }
    if(c->mode == 0) {
        dax_log(LOG_WARN, "Command Mode not given, assuming CONTINUOUS");
        c->mode = MB_CONTINUOUS;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "enable");
    if(lua_toboolean(L, -1)) {
        c->enable = 1;
    } else {
        c->enable = 0;
    }
    lua_pop(L, 1);

    /* We'll need ip address and port if we are a TCP port */
    if(port->protocol == MB_TCP) {
        lua_getfield(L, -1, "ipaddress");
        string = lua_tostring(L, -1);
        if(string != NULL) {
            result = inet_aton(string, &c->ip_address);
        }
        /* inet_aton will return 0 if the address is malformed */
        if(string == NULL || !result) {
            dax_log(LOG_INFO, "Using Default IP address of '127.0.0.1'\n");
            inet_aton("127.0.0.1", &c->ip_address);
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "port");
        c->port = lua_tonumber(L, -1);
        if(c->port == 0) {
            c->port = 502; /* 502 is the default modbus port */
        }
        lua_pop(L, 1);
    }

    lua_getfield(L, -1, "tagname");
    string = (char *)lua_tostring(L, -1);
    if(string != NULL) {
        c->data_tag = strdup(string);
    } else {
        dax_log(LOG_ERROR, "No Tagname Given for Command on Port %d", p);
    }
    lua_pop(L,1);
    lua_getfield(L, -1, "tagcount");
    c->tagcount = lua_tointeger(L, -1);
    if(c->tagcount == 0) {
        dax_log(LOG_ERROR, "No tag count given.  Using 1 as default");
        c->tagcount = 1;
    }
    lua_pop(L,1);

    if(c->mode & MB_TRIGGER) {
        lua_getfield(L, -1, "trigger");
        string = (char *)lua_tostring(L, -1);
        if(string != NULL) {
            c->trigger_tag = strdup(string);
        } else {
            dax_log(LOG_ERROR, "No Tagname Given for Trigger on Port %d", p);
            c->mode &= ~MB_TRIGGER;
        }
        lua_pop(L,1);
    }

    lua_getfield(L, -1, "interval");
    mb_set_interval(c, (int)lua_tonumber(L, -1));
    lua_pop(L,1);
    return 0;
}

/* This returns a pointer to the node stucture for the given port.
   Allocates the node if that has not been done yet.  Returns NULL
   on failure and a valid pointer otherwise */
static mb_node_def *
_get_node(mb_port *port, int nodeid) {

    if(port->nodes[nodeid] == NULL) { /* If it hasn't been allocated yet */
        port->nodes[nodeid] = malloc(sizeof(mb_node_def));
        if(port->nodes[nodeid] != NULL) {

            /* Initialize 'name' to NULL so later we can know if it was set */
            port->nodes[nodeid]->hold_name = NULL;
            port->nodes[nodeid]->input_name = NULL;
            port->nodes[nodeid]->coil_name = NULL;
            port->nodes[nodeid]->disc_name = NULL;
            port->nodes[nodeid]->hold_size = 0;
            port->nodes[nodeid]->input_size = 0;
            port->nodes[nodeid]->coil_size = 0;
            port->nodes[nodeid]->disc_size = 0;
            port->nodes[nodeid]->read_callback = LUA_REFNIL;
            port->nodes[nodeid]->write_callback = LUA_REFNIL;
        }
    }
    return port->nodes[nodeid];

}

/* Lua interface function for adding a modbus slave register tag
   to a port.  Accepts five arguments.
   Arguements:
      port id
      node / unit id
      tag name
      size (in registers or bits)
      register type [COIL, DISCRETE, HOLDING or INPUT]. */
static int
_add_register(lua_State *L)
{
    const char *tagname;
    int mbreg;
    unsigned int size;
    int nodeid;
    int p;
    mb_port *port;
    mb_node_def *node;

    p = lua_tointeger(L, 1);
    p--; /* Lua has indexes that are 1+ our actual array indexes */
    if(p < 0 || p >= config.portcount) {
        luaL_error(L, "Unknown Port ID : %d", p);
    }
    port = config.ports[p];
    if(port->type != MB_SLAVE) {
        dax_log(LOG_WARN, "Adding registers only makes sense for a Slave or Server port");
        return 0;
    }
    dax_log(LOG_DEBUG, "Adding a register to port %s", port->name);

    nodeid = lua_tointeger(L, 2);
    if(nodeid <0 || nodeid >= MB_MAX_SLAVE_NODES) {
        luaL_error(L, "Invalid node id given for register on Port %s", port->name);
    }

    tagname = (char *)lua_tostring(L, 3);
    if(tagname == NULL) {
        luaL_error(L, "No tagname Given for register on Port %s", port->name);
    }

    size = lua_tointeger(L, 4);
    if(size == 0 || size > 65535) {
        luaL_error(L, "Register size must be between 1-65535 on Port %s", port->name);
    }

    node = _get_node(port, nodeid);
    if(node == NULL) {
        luaL_error(L, "Unable to allocate memory for node on port %s", port->name);
    }

    mbreg = lua_tointeger(L, 5);
    switch(mbreg) {
        case MB_REG_HOLDING:
            node->hold_name = strdup(tagname);
            node->hold_size = size;
            break;
        case MB_REG_INPUT:
            node->input_name = strdup(tagname);
            node->input_size = size;
            break;
        case MB_REG_COIL:
            node->coil_name = strdup(tagname);
            node->coil_size = size;
            break;
        case MB_REG_DISC:
            node->disc_name = strdup(tagname);
            node->disc_size = size;
            break;
        default:
            luaL_error(L, "Invalid register type given on Port %s", port->name);
    }
    return 0;
}

/* Lua interface function for adding a modbus slave read callback function
   to a port.  Accepts five arguments.
   Arguements:
      port id
      node / unit id
      function
*/
static int
_add_read_callback(lua_State *L)
{
    int nodeid;
    int p;
    mb_port *port;
    mb_node_def *node;

    p = lua_tointeger(L, 1);
    p--; /* Lua has indexes that are 1+ our actual array indexes */
    if(p < 0 || p >= config.portcount) {
        luaL_error(L, "Unknown Port ID : %d", p);
    }
    port = config.ports[p];
    if(port->type != MB_SLAVE) {
        dax_log(LOG_WARN, "Adding registers only makes sense for a Slave or Server port");
        return 0;
    }
    dax_log(LOG_DEBUG, "Adding a register to port %s", port->name);

    nodeid = lua_tointeger(L, 2);
    if(nodeid <0 || nodeid >= MB_MAX_SLAVE_NODES) {
        luaL_error(L, "Invalid node id given for register on Port %s", port->name);
    }

    node = _get_node(port, nodeid);
    if(node == NULL) {
        luaL_error(L, "Unable to allocate memory for node on port %s", port->name);
    }

    lua_settop(L, 3); /*put the function at the top of the stack */
    if(! lua_isfunction(L, -1)) {
        luaL_error(L, "filter should be a function ");
    }
    /* Pop the function off the stack and write it to the regsitry and assign
       the reference to .filter */
    node->read_callback = luaL_ref(L, LUA_REGISTRYINDEX);

    return 0;
}

/* Lua interface function for adding a modbus slave read callback function
   to a port.  Accepts five arguments.
   Arguements:
      port id
      node / unit id
      function
*/
static int
_add_write_callback(lua_State *L)
{
    int nodeid;
    int p;
    mb_port *port;
    mb_node_def *node;

    p = lua_tointeger(L, 1);
    p--; /* Lua has indexes that are 1+ our actual array indexes */
    if(p < 0 || p >= config.portcount) {
        luaL_error(L, "Unknown Port ID : %d", p);
    }
    port = config.ports[p];
    if(port->type != MB_SLAVE) {
        dax_log(LOG_WARN, "Adding registers only makes sense for a Slave or Server port");
        return 0;
    }
    dax_log(LOG_DEBUG, "Adding a register to port %s", port->name);

    nodeid = lua_tointeger(L, 2);
    if(nodeid <0 || nodeid >= MB_MAX_SLAVE_NODES) {
        luaL_error(L, "Invalid node id given for register on Port %s", port->name);
    }

    node = _get_node(port, nodeid);
    if(node == NULL) {
        luaL_error(L, "Unable to allocate memory for node on port %s", port->name);
    }

    lua_settop(L, 3); /*put the function at the top of the stack */
    if(! lua_isfunction(L, -1)) {
        luaL_error(L, "filter should be a function ");
    }
    /* Pop the function off the stack and write it to the regsitry and assign
       the reference to .filter */
    node->write_callback = luaL_ref(L, LUA_REGISTRYINDEX);

    return 0;
}


/* This function should be called from main() to configure the program.
 * First the defaults are set then the configuration file is parsed then
 * the command line is handled.  This gives the command line priority.  */
int
modbus_configure(int argc, const char *argv[])
{
    int flags, result = 0;
    lua_State *L;

    _init_config();
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "tagname","tagname", 't', flags, "modbus");

    L = dax_get_luastate(ds);
    lua_pushinteger(L, MB_REG_HOLDING);
    lua_setglobal(L, "HOLDING");
    lua_pushinteger(L, MB_REG_INPUT);
    lua_setglobal(L, "INPUT");
    lua_pushinteger(L, MB_REG_COIL);
    lua_setglobal(L, "COIL");
    lua_pushinteger(L, MB_REG_DISC);
    lua_setglobal(L, "DISCRETE");

    dax_set_luafunction(ds, (void *)_add_port, "add_port");
    dax_set_luafunction(ds, (void *)_add_command, "add_command");
    dax_set_luafunction(ds, (void *)_add_register, "add_register");
    dax_set_luafunction(ds, (void *)_add_read_callback, "add_read_callback");
    dax_set_luafunction(ds, (void *)_add_write_callback, "add_write_callback");

    result = dax_configure(ds, argc, (char **)argv, CFG_CMDLINE | CFG_MODCONF);

    dax_clear_luafunction(ds, "add_port");
    dax_clear_luafunction(ds, "add_command");
    dax_clear_luafunction(ds, "add_register");
    dax_clear_luafunction(ds, "add_read_callback");
    dax_clear_luafunction(ds, "add_write_callback");

    /* Add functions that make sense for any callbacks that might be configured.
       The callback functions will live in the configuration Lua state */
    daxlua_register_function(L, "tag_get");
    daxlua_register_function(L, "tag_handle");
    daxlua_register_function(L, "tag_read");
    daxlua_register_function(L, "tag_write");
    /* TODO: Should add atomic operations */
    daxlua_register_function(L, "log");
    daxlua_register_function(L, "sleep");

    daxlua_set_constants(L);
    /* register the libraries that we need*/
    luaL_requiref(L, "_G", luaopen_base, 1);
    luaL_requiref(L, "math", luaopen_math, 1);
    luaL_requiref(L, "table", luaopen_table, 1);
    luaL_requiref(L, "string", luaopen_string, 1);
    luaL_requiref(L, "utf8", luaopen_utf8, 1);
    lua_pop(L, 5);


    printconfig();

    return result;
}


/* TODO: Really should print out more than this*/
static void
printconfig(void)
{
    int n;

    fprintf(stderr, "\n----------Modbus Configuration-----------\n\n");
    for(n=0; n<config.portcount; n++) {
        mb_print_portconfig(stderr, config.ports[n]);
    }

}
