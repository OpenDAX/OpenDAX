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
//#include <database.h>
#include <opendax.h>

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
        dax_debug(ds, 1, "No device given for serial port %s, Using /dev/serial", p->name);
        device = strdup("/dev/serial");
    }

    lua_getfield(L, -2, "baudrate");
    baudrate = (int)lua_tonumber(L, -1);
    if(baudrate == 0) {
        dax_debug(ds, 1, "Unknown Baudrate, Using 9600");
        baudrate = 9600;
    }

    lua_getfield(L, -3, "databits");
    databits = (short)lua_tonumber(L, -1);
    if(databits < 7 || databits > 8) {
        dax_debug(ds, 1, "Unknown databits - %d, Using 8", databits);
        databits = 8;
    }

    lua_getfield(L, -4, "stopbits");
    stopbits = (unsigned int)lua_tonumber(L, -1);
    if(stopbits != 1 && stopbits != 2) {
        dax_debug(ds, 1, "Unknown stopbits - %d, Using 1", stopbits);
        stopbits = 1;
    }

    lua_getfield(L, -5, "parity");
    if(lua_isnumber(L, -1)) {
        parity = (unsigned char)lua_tonumber(L, -1);
        if(parity != MB_ODD && parity != MB_EVEN && parity != MB_NONE) {
            dax_debug(ds, 1, "Unknown Parity %d, using NONE", parity);
        }
    } else {
        string = (char *)lua_tostring(L, -1);
        if(string) {
            if(strcasecmp(string, "NONE") == 0) parity = MB_NONE;
            else if(strcasecmp(string, "EVEN") == 0) parity = MB_EVEN;
            else if(strcasecmp(string, "ODD") == 0) parity = MB_ODD;
            else {
                dax_debug(ds, 1, "Unknown Parity %s, using NONE", string);
                parity = MB_NONE;
            }
        } else {
            dax_debug(ds, 1, "Parity not given, using NONE");
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
            dax_debug(ds, 1, "Unknown Socket Type %s, using TCP", string);
            socket = TCP_SOCK;
        }
    } else {
        dax_debug(ds, 1, "Socket Type not given, using TCP");
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

static inline int
_get_slave_config(lua_State *L, mb_port *p)
{
    unsigned int size;
    int result = 0;
    dax_error(ds, "Slave functionality is not yet implemented");
    char *reg_name;

    lua_getfield(L, -1, "holdreg");
    reg_name = (char *)lua_tostring(L, -1);
    lua_getfield(L, -2, "holdsize");
    size = (unsigned int)lua_tonumber(L, -1);
    if(reg_name && size) {
        p->hold_name = strdup(reg_name);
        if(p->hold_name == NULL) return ERR_ALLOC;
        result = mb_set_holdreg_size(p, size);
        if(result) return result;
    }
    lua_pop(L, 2);

    lua_getfield(L, -1, "inputreg");
    reg_name = (char *)lua_tostring(L, -1);
    lua_getfield(L, -2, "inputsize");
    size = (unsigned int)lua_tonumber(L, -1);
    if(reg_name && size) {
        p->input_name = strdup(reg_name);
        if(p->input_name == NULL) return ERR_ALLOC;
        result = mb_set_inputreg_size(p, size);
        if(result) return result;
    }
    lua_pop(L, 2);

    lua_getfield(L, -1, "coilreg");
    reg_name = (char *)lua_tostring(L, -1);
    lua_getfield(L, -2, "coilsize");
    size = (unsigned int)lua_tonumber(L, -1);
    if(reg_name && size) {
        p->coil_name = strdup(reg_name);
        if(p->coil_name == NULL) return ERR_ALLOC;
        result = mb_set_coil_size(p, size);
        if(result) return result;
    }
    lua_pop(L, 2);

    lua_getfield(L, -1, "discreg");
    reg_name = (char *)lua_tostring(L, -1);
    lua_getfield(L, -2, "discsize");
    size = (unsigned int)lua_tonumber(L, -1);
    if(reg_name && size) {
        p->disc_name = strdup(reg_name);
        if(p->disc_name == NULL) return ERR_ALLOC;
        result = mb_set_discrete_size(p, size);
        if(result) return result;
    }
    lua_pop(L, 2);

    return 0;
}

/* Lua interface function for adding a port.  It takes a single
   table as an argument and returns the Port's index */
static int
_add_port(lua_State *L)
{
    mb_port *p;
    mb_port **newports;
    char *string, *name;
    int slaveid, tmp, maxfailures, inhibit;
    unsigned char devtype, protocol, type;
    
    if(!lua_istable(L, -1)) {
        luaL_error(L, "add_port() received an argument that is not a table");
    }
    
    /* This logic allocates the port array if it does not already exist */
    if(config.ports == NULL) {
        config.ports = malloc(sizeof(mb_port *) * DEFAULT_PORTS);
        config.portsize = DEFAULT_PORTS;
        if(config.ports == NULL) {
            dax_fatal(ds, "Unable to allocate port array");
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
            dax_fatal(ds, "Unable to reallocate port array");
        }
    }
    
    dax_debug(ds, LOG_MINOR, "Adding a port at index = %d", config.portcount);
    
    lua_getfield(L, -1, "name");
    name = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);
    
    config.ports[config.portcount] = mb_new_port(name, 0x0000);
    /* Assign the pointer to p to make things simpler */
    p = config.ports[config.portcount];
    if(p == NULL) {
        dax_fatal(ds, "Unable to allocate port[%d]", config.portcount);
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
            dax_debug(ds, 1, "Unknown Protocol %s, assuming RTU", string);
            protocol = MB_RTU;
            devtype = SERIAL_PORT;
        }
    } else {
        dax_debug(ds, 1, "Protocol not given, assuming RTU");
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
        dax_debug(ds, 1, "Unknown Port Type %s, assuming MASTER", string);
        type = MB_MASTER;
    }
    lua_pop(L, 1);
    if(type == MB_SLAVE) {
        lua_getfield(L, -1, "slaveid");
        slaveid = (int)lua_tonumber(L, -1);
        lua_pop(L, 1);
       _get_slave_config(L, p);
    } else {
        slaveid = 0;
    }

    mb_set_protocol(p, type, protocol, slaveid);
    
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
        luaL_error(L, "Unknown Port ID : %d", p);
    }
    if(!lua_istable(L, 2)) {
        luaL_error(L, "add_command() received an argument that is not a table");
    }
    port = config.ports[p];
    if(port->type != MB_MASTER) {
        dax_debug(ds, 1, "Adding commands only makes sense for a Master or Client port");
        return 0;
    }
    dax_debug(ds, LOG_CONFIG, "Adding a command to port %d", p);
    
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
        dax_error(ds, "Unable to set command");
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
                dax_error(ds, "ON CHANGE command trigger only makes sense for write commands");
            }
        }
        if(strcasestr(string, "WRITE") != NULL) {
            if(mb_is_write_cmd(c)) {
                c->mode |= MB_ONWRITE;
            } else {
                dax_error(ds, "ON WRITE command trigger only makes sense for write commands");
            }
        }
    }
    if(c->mode == 0) {
        dax_debug(ds, 1, "Command Mode not given, assuming CONTINUOUS");
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
            dax_debug(ds, LOG_CONFIG, "Using Default IP address of '127.0.0.1'\n");
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
        dax_error(ds, "No Tagname Given for Command on Port %d", p);
    }
    lua_pop(L,1);
    lua_getfield(L, -1, "tagcount");
    c->tagcount = lua_tointeger(L, -1);
    if(c->tagcount == 0) {
        dax_error(ds, "No tag count given.  Using 1 as default");
        c->tagcount = 1;
    }
    lua_pop(L,1);

    if(c->mode & MB_TRIGGER) {
        lua_getfield(L, -1, "trigger");
        string = (char *)lua_tostring(L, -1);
        if(string != NULL) {
            c->trigger_tag = strdup(string);
        } else {
            dax_error(ds, "No Tagname Given for Trigger on Port %d", p);
            c->mode &= ~MB_TRIGGER;
        }
        lua_pop(L,1);
    }

    lua_getfield(L, -1, "interval");
    mb_set_interval(c, (int)lua_tonumber(L, -1));
    lua_pop(L,1);
    return 0;
}

/* This function should be called from main() to configure the program.
 * First the defaults are set then the configuration file is parsed then
 * the command line is handled.  This gives the command line priority.  */
int
modbus_configure(int argc, const char *argv[])
{
    int flags, result = 0;
    
    _init_config();
    dax_init_config(ds, "modbus");
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "tagname","tagname", 't', flags, "modbus");
    
    dax_set_luafunction(ds, (void *)_add_port, "add_port");
    dax_set_luafunction(ds, (void *)_add_command, "add_command");
    
    dax_configure(ds, argc, (char **)argv, CFG_CMDLINE | CFG_MODCONF);

    dax_clear_luafunction(ds, "add_port");
    dax_clear_luafunction(ds, "add_command");

    dax_free_config(ds);
    
    printconfig();

    return 0;
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
