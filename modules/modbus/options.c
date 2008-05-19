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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <termios.h>

#include <common.h>
#include <options.h>
#include <modbus.h>
#include <opendax.h>

static void printconfig(void);

struct Config config;

/* Inititialize the configuration to NULL or 0 for cleanliness */
static void initconfig(void) {

    config.pidfile = NULL;
    config.tagname = NULL;
    config.tablesize = 0;
    config.verbosity = 0;
    config.daemonize = 0;
    config.ports = NULL;
        
    asprintf(&config.configfile, "%s/%s", ETC_DIR, "modbus.conf");
}

/* This function parses the command line options and sets
 the proper members of the configuration structure */
static void parsecommandline(int argc, const char *argv[])  {
    char c;
    
    static struct option options[] = {
        {"config", required_argument, 0, 'C'},
        {"version", no_argument, 0, 'V'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    /* Get the command line arguments */ 
    while ((c = getopt_long (argc, (char * const *)argv, "C:VvD",options, NULL)) != -1) {
        switch (c) {
            case 'C':
                config.configfile = strdup(optarg);
                break;
            case 'V':
                printf("OpenDAX Modbus Module Version %s\n", VERSION);
                break;
            case 'v':
                config.verbosity++;
                break;
            case 'D': 
                config.daemonize = 1;
                break;
            case '?':
                printf("Got the big ?\n");
                break;
            case -1:
            case 0:
                break;
            default:
                printf ("?? getopt returned character code 0%o ??\n", c);
        } /* End Switch */
    } /* End While */           
}

static inline int
get_serial_config(lua_State *L, struct mb_port *p)
{
    char *string;
    
    lua_getfield(L, -1, "device");
    string = (char *)lua_tostring(L, -1);
    if(string == NULL) {
        luaL_error(L, "No device given for serial port %s", p->name);
    } else {
        p->device = strdup(string);
    }
    
    lua_getfield(L, -2, "baudrate");
    p->baudrate = (unsigned int)lua_tonumber(L, -1);
    if(getbaudrate(p->baudrate) == 0) {
        dax_debug(1, "Unknown Baudrate - %s, Using 9600", lua_tostring(L, -1));
        p->baudrate=9600;
    }
    
    lua_getfield(L, -3, "databits");
    p->databits = (unsigned int)lua_tonumber(L, -1);
    if(p->databits < 7 && p->databits > 8) {
        dax_debug(1, "Unknown databits - %d, Using 8", p->databits);
        p->databits=8;
    }

    lua_getfield(L, -4, "stopbits");
    p->stopbits = (unsigned int)lua_tonumber(L, -1);
    if(p->stopbits !=1 && p->stopbits != 2) {
        dax_debug(1, "Unknown stopbits - %d, Using 1", p->stopbits);
        p->stopbits=8;
    }
    
    lua_getfield(L, -5, "parity");
    if(lua_isnumber(L, -1)) {
        p->parity = (unsigned char)lua_tonumber(L, -1);
        if(p->parity != ODD && p->parity != EVEN && p->parity != NONE) {
            dax_debug(1, "Unknown Parity %d, using NONE", p->parity);
        }
    } else {
        string = (char *)lua_tostring(L, -1);
        if(string) {
            if(strcasecmp(string, "NONE") == 0) p->parity = NONE;
            else if(strcasecmp(string, "EVEN") == 0) p->parity = EVEN;
            else if(strcasecmp(string, "ODD") == 0) p->parity = ODD;
            else {
                dax_debug(1, "Unknown Parity %s, using NONE", string);
                p->parity = NONE;
            }
        } else {
            dax_debug(1, "Parity not given, using NONE");
            p->parity = NONE;
        }
    }
    lua_pop(L, 5);    
    return 0;
}

static inline int
get_network_config(lua_State *L, struct mb_port *p)
{
    char *string;
    
    lua_getfield(L, -1, "ipaddress");
    string = (char *)lua_tostring(L, -1);
    if(string == NULL) {
        dax_debug(1, "No ipaddress given for port %s, using default", p->name);
        strcpy(p->ipaddress, "0.0.0.0");
    } else {
        strncpy(p->ipaddress, string, 15);
    }
    
    lua_getfield(L, -2, "bindport");
    p->bindport = (unsigned int)lua_tonumber(L, -1);
    if(p->bindport == 0) p->bindport = 5001;
    
    lua_getfield(L, -3, "socket");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "TCP") == 0) p->socket = TCP_SOCK;
        else if(strcasecmp(string, "UDP") == 0) p->socket = UDP_SOCK;
        else {
            dax_debug(1, "Unknown Socket Type %s, using TCP", string);
            p->socket = TCP_SOCK;
        }
    } else {
        dax_debug(1, "Socket Type not given, using TCP");
        p->socket = TCP_SOCK;
    }
    lua_pop(L, 3);
    return 0;
}

static inline int
get_slave_config(lua_State *L, struct mb_port *p)
{
    dax_error("Slave functionality is not yet implemented");
    
    lua_getfield(L, -1, "holdreg");
    p->holdreg = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -2, "holdsize");
    p->holdsize = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -3, "inputreg");
    p->inputreg = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -4, "inputsize");
    p->inputsize = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -5, "coilreg");
    p->coilreg = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -6, "coilsize");
    p->coilsize = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -7, "floatreg");
    p->floatreg = (unsigned int)lua_tonumber(L, -1);
    lua_getfield(L, -8, "floatsize");
    p->floatsize = (unsigned int)lua_tonumber(L, -1);
    lua_pop(L, 8);
    
    return 0;
}

/* Lua interface function for adding a port.  It takes a single
   table as an argument and returns the Port's index */
static int
_add_port(lua_State *L)
{
    struct mb_port *p;
    char *string;
    
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
        config.ports = (struct mb_port *)malloc(sizeof(struct mb_port) * config.maxports);
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
    p->name = (char *)lua_tostring(L, -1);
    
    lua_getfield(L, -2, "enable");
    p->enable = (char)lua_toboolean(L, -1);
    
    /* The devtype is the type of device we are going to use to communicte.
     right now its either a serial port or a network socket */
    lua_getfield(L, -3, "devtype");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "SERIAL") == 0) p->devtype = SERIAL;
        else if(strcasecmp(string, "NET") == 0) p->devtype = NET;
        else {
            dax_debug(1, "Unknown Device Type %s, assuming SERIAL", string);
            p->devtype = SERIAL;
        }
    } else {
        dax_debug(1, "Device Type not given, assuming SERIAL");
        p->devtype = SERIAL;
    }
        
    lua_pop(L, 3);
    /* Serial port and network configurations need different data.  These
     two functions will get the right stuff out of the table.  The table
     is not checked to see if too much information is given only that
     enough information is given to do the job */
    if(p->devtype == SERIAL) {
        get_serial_config(L, p);
    } else if(p->devtype == NET) {
        get_network_config(L, p);
    }
    /* What Modbus protocol are we going to talk on this port */
    lua_getfield(L, -1, "protocol");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        if(strcasecmp(string, "RTU") == 0) p->protocol = RTU;
        else if(strcasecmp(string, "ASCII") == 0) p->protocol = ASCII;
        else if(strcasecmp(string, "TCP") == 0) p->protocol = TCP;
        else {
            dax_debug(1, "Unknown Protocol %s, assuming RTU", string);
            p->protocol = RTU;
        }
    } else {
        dax_debug(1, "Protocol not given, assuming RTU");
        p->protocol = RTU;
    }
    lua_getfield(L, -2, "type");
    string = (char *)lua_tostring(L, -1);
    if(strcasecmp(string, "MASTER") == 0) p->type = MASTER;
    else if(strcasecmp(string, "SLAVE") == 0) p->type = SLAVE;
    else if(strcasecmp(string, "CLIENT") == 0) p->type = MASTER;
    else if(strcasecmp(string, "SERVER") == 0) p->type = SLAVE;
    
    else {
        dax_debug(1, "Unknown Port Type %s, assuming MASTER", string);
        p->type = MASTER;
    }
    
    lua_pop(L, 2);
    if(p->type == SLAVE) get_slave_config(L, p);
    
    lua_getfield(L, -1, "delay");
    p->delay = (unsigned int)lua_tonumber(L, -1);
    
    lua_getfield(L, -2, "wait");
    p->wait = (unsigned int)lua_tonumber(L, -1);
    
    lua_getfield(L, -3, "scanrate");
    p->scanrate = (unsigned int)lua_tonumber(L, -1);
    if(p->scanrate == 0) p->scanrate = 100;
    
    lua_getfield(L, -4, "timeout");
    p->timeout = (unsigned int)lua_tonumber(L, -1);
    if(p->timeout == 0) p->timeout = 1000;
    
    lua_getfield(L, -5, "retries");
    p->retries = (unsigned int)lua_tonumber(L, -1);
    if(p->retries == 0) p->timeout = 1;
    if(p->retries > MAX_RETRIES) p->retries = MAX_RETRIES;
    
    lua_getfield(L, -6, "inhibit");
    p->inhibit_time = (unsigned int)lua_tonumber(L, -1);
    
    lua_getfield(L, -7, "maxattempts");
    p->maxattempts = (unsigned int)lua_tonumber(L, -1);
    
    lua_pop(L, 7);
    /* The lua script gets the index +1 */
    config.portcount++;
    lua_pushnumber(L, config.portcount);
    
    p->commands = NULL;
    p->running = 0;
	p->inhibit = 0;
	p->attempt = 0;
    p->inhibit_temp = 0;
	p->fd = 0;
    p->dienow = 0;
    p->out_callback = NULL;
    p->in_callback = NULL;
    
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
    if(config.ports[p].type != MASTER) {
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

static void
initscript(lua_State *L)
{
    /* We don't open any librarires because we don't really want any
     function calls in the configuration file.  It's just for
     setting a few globals.*/
    
    /* TODO: Need to set up some globals for configuration.
        Maybe I'll use strings instead. 
        stuff like SLAVE, MASTER, RTU, ASCII, TCP, UDP, EVEN, ODD */
    
    lua_pushcfunction(L, _add_port);
    lua_setglobal(L, "add_port");
    lua_pushcfunction(L, _add_command);
    lua_setglobal(L, "add_command");
    
}

static int
readconfigfile(void)
{
    lua_State *L;
    char *string;
    
    L = lua_open();

    initscript(L);
    
    /* load and run the configuration file */
    if(luaL_loadfile(L, config.configfile)  || lua_pcall(L, 0, 0, 0)) {
        dax_error("Problem executing configuration file - %s", lua_tostring(L, -1));
        return 1;
    }
    
    /* tell lua to push these variables onto the stack */
    lua_getglobal(L, "tagname");
    lua_getglobal(L, "verbosity");
    lua_getglobal(L, "tablesize");
       
    if(config.tagname == NULL) {
        if( (string = (char *)lua_tostring(L, 1)) ) {
            config.tagname = strdup(string);
        }
    }
    
    if(config.verbosity == 0) { /* Make sure we didn't get anything on the commandline */
        config.verbosity = (int)lua_tonumber(L, 3);
        dax_set_verbosity(config.verbosity);
    }
    
    if(config.tablesize == 0) { /* Make sure we didn't get anything on the commandline */
        config.tablesize = (unsigned int)lua_tonumber(L, 3);
    }
    
    /* Clean up and get out */
    lua_close(L);
    
    return 0;
}

/* This function sets the defaults for the configuration if the
 commandline or the config file set them. */
static void
setdefaults(void)
{
    if(!config.tagname) {
        config.tagname = strdup(DEFAULT_TAGNAME);
    }
    if(!config.tablesize)
        config.tablesize = DEFAULT_TABLE_SIZE;
}


/* This function should be called from main() to configure the program.
   First the defaults are set then the configuration file is parsed then
   the command line is handled.  This gives the command line priority.  */
int
modbus_configure(int argc, const char *argv[])
{
    initconfig();
    parsecommandline(argc, argv);
    if(config.verbosity > 0) dax_set_verbosity(config.verbosity);
    if(readconfigfile()) {
        dax_error("Unable to read configuration running with defaults");
    }
    setdefaults();
    if(config.verbosity >= 5) printconfig();
    return 0;
}

int
getbaudrate(int b_in)
{
    switch(b_in) {
        case 300:
            return B300;
        case 600:
            return B600;
        case 1200:
            return B1200;
        case 1800:
            return B1800;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        default:
            return 0;
    }
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
        if(config.ports[n].devtype == NET) {
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
