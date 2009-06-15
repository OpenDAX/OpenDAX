/* Mbd - Modbus (tm) Communications Program
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
 */

#include <common.h>
#include <modopt.h>
#include <modbus.h>
#include <opendax.h>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <getopt.h>
#include <termios.h>


static void printconfig(void);

struct Config config;

static inline int
_get_serial_config(lua_State *L, mb_port *p)
{
    char *string;
    char *device;
    u_int8_t slaveid;
    int baudrate;
    short databits;
    short stopbits;
    short parity;
    int result;

    lua_getfield(L, -1, "device");
    device = (char *)lua_tostring(L, -1);
    if(string == NULL) {
        luaL_error(L, "No device given for serial port %s", p->name);
    }
    
    lua_getfield(L, -2, "baudrate");
    baudrate = (int)lua_tonumber(L, -1);
    if(baudrate == 0) {
        dax_debug(1, "Unknown Baudrate - %s, Using 9600", lua_tostring(L, -1));
        baudrate = 9600;
    }
    
    lua_getfield(L, -3, "databits");
    databits = (short)lua_tonumber(L, -1);
    if(databits < 7 && databits > 8) {
        dax_debug(1, "Unknown databits - %d, Using 8", databits);
        p->databits=8;
    }

    lua_getfield(L, -4, "stopbits");
    stopbits = (unsigned int)lua_tonumber(L, -1);
    if(stopbits !=1 && stopbits != 2) {
        dax_debug(1, "Unknown stopbits - %d, Using 1", stopbits);
        stopbits=8;
    }
    
    lua_getfield(L, -5, "parity");
    if(lua_isnumber(L, -1)) {
        parity = (unsigned char)lua_tonumber(L, -1);
        if(parity != MB_ODD && parity != MB_EVEN && parity != MB_NONE) {
            dax_debug(1, "Unknown Parity %d, using NONE", parity);
        }
    } else {
        string = (char *)lua_tostring(L, -1);
        if(string) {
            if(strcasecmp(string, "NONE") == 0) parity = MB_NONE;
            else if(strcasecmp(string, "EVEN") == 0) parity = MB_EVEN;
            else if(strcasecmp(string, "ODD") == 0) parity = MB_ODD;
            else {
                dax_debug(1, "Unknown Parity %s, using NONE", string);
                parity = MB_NONE;
            }
        } else {
            dax_debug(1, "Parity not given, using NONE");
            parity = MB_NONE;
        }
    }
    result = mb_set_serial_port(p, device, baudrate, databits, parity, stopbits);
    lua_pop(L, 5);    
    return result;
;
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
    //if(ipaddress == NULL) {
        //dax_debug(1, "No ipaddress given for port %s, using default", p->name);
        //--strcpy(p->ipaddress, "0.0.0.0");
    //} else {
        ///strncpy(p->ipaddress, string, 15);
    //}
    
    lua_getfield(L, -2, "bindport");
    bindport = (unsigned int)lua_tonumber(L, -1);
    if(bindport == 0) bindport = 502;
    
    lua_getfield(L, -3, "socket");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "TCP") == 0) socket = TCP_SOCK;
        else if(strcasecmp(string, "UDP") == 0) socket = UDP_SOCK;
        else {
            dax_debug(1, "Unknown Socket Type %s, using TCP", string);
            socket = TCP_SOCK;
        }
    } else {
        dax_debug(1, "Socket Type not given, using TCP");
        socket = TCP_SOCK;
    }
    if(ipaddress != NULL) {
        result = mb_set_network_port(p, ipaddress, bindport, socket);
    } else {
        result = mb_set_network_port(p, "0.0.0.0", bindport, socket);        
    }
    lua_pop(L, 3);
    return result;
}

static inline int
_get_slave_config(lua_State *L, mb_port *p)
{
    unsigned int size;
    int result;
    dax_error("Slave functionality is not yet implemented");
    
    //--lua_getfield(L, -1, "holdreg");
    //--holdreg = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -2, "holdsize");
    size = (unsigned int)lua_tonumber(L, -1);
    result = mb_set_holdsize(p, size);
    lua_pop(L, 1);
    if(result) return result;
    
    //--lua_getfield(L, -3, "inputreg");
    //--p->inputreg = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -4, "inputsize");
    size = (unsigned int)lua_tonumber(L, -1);
    result = mb_set_inputsize(p, size);
    lua_pop(L, 1);
    if(result) return result;
    
    lua_getfield(L, -5, "coilreg");
    //--p->coilreg = (unsigned int)lua_tonumber(L, -1);
    //--lua_getfield(L, -6, "coilsize");
    size = (unsigned int)lua_tonumber(L, -1);
    result = mb_set_coilsize(p, size);
    lua_pop(L, 1);
    if(result) return result;
    lua_getfield(L, -7, "floatreg");
    
    //--p->floatreg = (unsigned int)lua_tonumber(L, -1);
    //--lua_getfield(L, -8, "floatsize");
    size = (unsigned int)lua_tonumber(L, -1);
    result = mb_set_floatsize(p, size);
    lua_pop(L, 1);
    if(result) return result;
    
    return 0;
}

/* Lua interface function for adding a port.  It takes a single
   table as an argument and returns the Port's index */
static int
_add_port(lua_State *L)
{
    mb_port *p;
    char *string;
    unsigned char devtype, protocol, type; 
    
    if(!lua_istable(L, -1)) {
        luaL_error(L, "add_port() received an argument that is not a table");
    }
    
    /* This logic allocates the port array if it does not already exist */
    if(config.ports == NULL) {
        lua_getglobal(L, "maxports");
        if(config.maxports == 0) {
            config.maxports = (int)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        if(config.maxports <= 0) {
            config.maxports = MAX_PORTS;
        }
        dax_debug(10, "Allocating %d ports", config.maxports);
        //--NO GOOD MUST USE NEW FUNCTION TO CREATE A PORT
        //--config.ports = (struct mb_port *)malloc(sizeof(struct mb_port) * config.maxports);
        if(config.ports == NULL) {
            dax_fatal("Unable to allocate port array");
        }
    }
    /* Check to makes sure that we have some ports left */
    if(config.portcount >= config.maxports) {
        dax_error("Trying to add too many ports.  Maximum is %d", config.maxports);
        return 0;
    }
    dax_debug(8, "Adding a port at index = %d", config.portcount);
    /* Assign the pointer to p to make things simpler */
    p = &config.ports[config.portcount];
    
    lua_getfield(L, -1, "name");
    name = (char *)lua_tostring(L, -1);
    
    lua_getfield(L, -2, "enable");
    enable = (char)lua_toboolean(L, -1);
    
    /* The devtype is the type of device we are going to use to communicte.
     right now its either a serial port or a network socket */
    lua_getfield(L, -3, "devtype");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "SERIAL") == 0) devtype = MB_SERIAL;
        else if(strcasecmp(string, "NET") == 0) devtype = MB_NETWORK;
        else {
            dax_debug(1, "Unknown Device Type %s, assuming SERIAL", string);
            devtype = MB_SERIAL;
        }
    } else {
        dax_debug(1, "Device Type not given, assuming SERIAL");
        devtype = MB_SERIAL;
    }
        
    lua_pop(L, 3);
    /* Serial port and network configurations need different data.  These
     two functions will get the right stuff out of the table.  The table
     is not checked to see if too much information is given only that
     enough information is given to do the job */
    if(devtype == MB_SERIAL) {
        _get_serial_config(L, p);
    } else if(devtype == MB_NETWORK) {
        _get_network_config(L, p);
    }
    /* What Modbus protocol are we going to talk on this port */
    lua_getfield(L, -1, "protocol");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "RTU") == 0) protocol = MB_RTU;
        else if(strcasecmp(string, "ASCII") == 0) protocol = MB_ASCII;
        else if(strcasecmp(string, "TCP") == 0) protocol = MB_TCP;
        else {
            dax_debug(1, "Unknown Protocol %s, assuming RTU", string);
            protocol = MB_RTU;
        }
    } else {
        dax_debug(1, "Protocol not given, assuming RTU");
        protocol = MB_RTU;
    }
    lua_getfield(L, -2, "type");
    string = (char *)lua_tostring(L, -1);
    if(strcasecmp(string, "MASTER") == 0) type = MB_MASTER;
    else if(strcasecmp(string, "SLAVE") == 0) type = MB_SLAVE;
    else if(strcasecmp(string, "CLIENT") == 0) type = MB_MASTER;
    else if(strcasecmp(string, "SERVER") == 0) type = MB_SLAVE;
    
    else {
        dax_debug(1, "Unknown Port Type %s, assuming MASTER", string);
        type = MB_MASTER;
    }
    
    lua_pop(L, 2);
    if(type == MB_SLAVE) _get_slave_config(L, p);
    
    /* Have to decide how much of this will really be needed */
//    lua_getfield(L, -1, "delay");
//    p->delay = (unsigned int)lua_tonumber(L, -1);
//    
//    lua_getfield(L, -2, "wait");
//    p->wait = (unsigned int)lua_tonumber(L, -1);
//    
//    lua_getfield(L, -3, "scanrate");
//    p->scanrate = (unsigned int)lua_tonumber(L, -1);
//    if(p->scanrate == 0) p->scanrate = 100;
//    
//    lua_getfield(L, -4, "timeout");
//    p->timeout = (unsigned int)lua_tonumber(L, -1);
//    if(p->timeout == 0) p->timeout = 1000;
//    
//    lua_getfield(L, -5, "retries");
//    p->retries = (unsigned int)lua_tonumber(L, -1);
//    if(p->retries == 0) p->timeout = 1;
//    if(p->retries > MAX_RETRIES) p->retries = MAX_RETRIES;
//    
//    lua_getfield(L, -6, "inhibit");
//    p->inhibit_time = (unsigned int)lua_tonumber(L, -1);
//    
//    lua_getfield(L, -7, "maxattempts");
//    p->maxattempts = (unsigned int)lua_tonumber(L, -1);
    
//    lua_pop(L, 7);
    /* The lua script gets the index +1 */
    config.portcount++;
    lua_pushnumber(L, config.portcount);
    
    return 1;
}

/* Lua interface function for adding a modbus command to a port.  
   Accepts two arguments.  The first is the port to assign the command
   too, and the second is a table describing the command. */
static int
_add_command(lua_State *L)
{
    struct mb_cmd *c;
    char *string;
    int p; /* Port ID */
    
    p = (int)lua_tonumber(L, 1);
    p--; /* Lua has indexes that are 1+ our actual array indexes */
    if(p < 0 || p >= config.maxports) {
        luaL_error(L, "Unknown Port ID");
    }
    if(!lua_istable(L, 2)) {
        luaL_error(L, "add_command() received an argument that is not a table");
    }
    if(config.ports[p].type != MB_MASTER) {
        dax_debug(1, "Adding commands only makes sense for a Master or Client port");
        return 0;
    }
    dax_debug(10, "Adding a command to port %d", p);
    
    /* Allocate the new command and add it to the port */
    c = mb_new_cmd();
    if(c == NULL) {
        luaL_error(L, "Can't allocate memory for the command");
    }
    mb_add_cmd(&config.ports[p], c);
    
    lua_getfield(L, -1, "method");
    string = (char *)lua_tostring(L, -1);
    if(string != NULL) {
        if(strcasecmp(string, "DISABLE") == 0) c->method = MB_DISABLE;
        else if(strncasecmp(string, "CONT", 4) == 0) c->method = MB_CONTINUOUS;
        else if(strcasecmp(string, "CHANGE") == 0) c->method = MB_ONCHANGE;
        else if(strcasecmp(string, "ONCHANGE") == 0) c->method = MB_ONCHANGE;
        else {
            dax_debug(1, "Unknown Command Method %s, assuming CONTINUOUS", string);
            c->method = MB_CONTINUOUS;
        }
    } else {
        dax_debug(1, "Command Method not given, assuming CONTINUOUS");
        c->method = MB_CONTINUOUS;
    }
    
    lua_getfield(L, -2, "node");
    c->node = (int)lua_tonumber(L, -1);
    
    lua_getfield(L, -3, "fcode");
    c->function = (int)lua_tonumber(L, -1);
    
    lua_getfield(L, -4, "register");
    c->m_register = (int)lua_tonumber(L, -1);
    
    lua_getfield(L, -5, "length");
    c->length = (int)lua_tonumber(L, -1);
    
    lua_getfield(L, -6, "length");
    c->length = (int)lua_tonumber(L, -1);
    
    lua_getfield(L, -7, "index");
    c->index = (int)lua_tonumber(L, -1);
    
    lua_getfield(L, -8, "tagname");
    string = (char *)lua_tostring(L, -1);
    if(string != NULL) {
        c->tagname = strdup(string);
        //dt_add_tag(string, c->index, c->function, c->length);
    } else {
        dax_debug(1, "No Tagname Given for Command on Port %d", p);
    }
    lua_getfield(L, -9, "interval");
    c->interval = (int)lua_tonumber(L, -1);
    
    return 0;
}

/* This function should be called from main() to configure the program.
 * First the defaults are set then the configuration file is parsed then
 * the command line is handled.  This gives the command line priority.  */
int
modbus_configure(int argc, const char *argv[])
{
    int flags, result = 0;
    
    dax_init_config("modbus");
    flags = CFG_CMDLINE | CFG_DAXCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute("tagname","tagname", 't', flags, "modbus");
    
    dax_set_luafunction((void *)_add_port, "add_port");
    dax_set_luafunction((void *)_add_command, "add_command");
    
    dax_configure(argc, (char **)argv, CFG_CMDLINE | CFG_DAXCONF | CFG_MODCONF);

    dax_free_config();
    
    printconfig();

    return 0;
}


/* TODO: Really should print out more than this*/
static void
printconfig(void)
{
    int n,i;
    struct mb_cmd *mc;
    printf("\n----------mbd Configuration-----------\n\n");
    printf("Configuration File: %s\n", config.configfile);
    printf("Table Size: %d\n", config.tablesize);
    printf("Maximum Ports: %d\n", config.maxports);
    printf("\n");
    for(n=0; n<config.portcount; n++) {
        if(config.ports[n].devtype == MB_NETWORK) {
            printf("Port[%d] %s %s:%d\n",n,config.ports[n].name,
                                             config.ports[n].ipaddress,
                                             config.ports[n].bindport);
        } else {
            printf("Port[%d] %s %s %d,%d,%d,%d\n",n,config.ports[n].name,
                                             config.ports[n].device,
                                             config.ports[n].baudrate,
                                             config.ports[n].databits,
                                             config.ports[n].parity,
                                             config.ports[n].stopbits);
        }
        mc = config.ports[n].commands;
        i = 0;
        while(mc != NULL) {
            printf(" Command[%d] %d %d %d %d %s %d\n",i++,mc->node,
                                                  mc->function,
                                                  mc->m_register,
                                                  mc->length,
                                                  mc->tagname,
                                                  mc->index);
            mc = mc->next;
        }
    }
}
