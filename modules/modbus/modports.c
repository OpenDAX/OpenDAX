/* modports.c - Modbus (tm) Communications Library
 * Copyright (C) 2009 Phil Birkelbach
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
 * Source file for mb_port handling functions
 */
 
#include <modbus.h>
#include <modlib.h>

/* Initializes the port structure given by pointer p */
static void
initport(mb_port *p)
{
    p->name = NULL;
    p->device = NULL;
    p->enable = 0;
    p->type = 0; 
    p->protocol = 0;
    p->devtype = MB_SERIAL;
    p->slaveid = 1;      
    p->baudrate = B9600;
    p->databits = 8;
    p->stopbits = 1;
    p->timeout = 1000;      
    p->wait = 10;
    p->delay = 0;     
    p->retries = 3;
    p->parity = MB_NONE;  
    p->bindport = 5001;
    p->scanrate = 1000; 
    p->holdreg = NULL;  
    p->holdsize = 0; 
    p->inputreg = NULL;
    p->inputsize = 0;
    p->coilreg = NULL;
    p->coilsize = 0;
    p->floatreg = NULL;
    p->floatsize = 0;
    p->running = 0;
    p->inhibit = 0;
    p->commands = NULL;
    p->out_callback = NULL;
    p->in_callback = NULL;
    strcpy(p->ipaddress, "0.0.0.0");
};

static int
getbaudrate(unsigned int b_in)
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

/********************/
/* Public Functions */
/********************/

/* This allocates and initializes a port.  Returns the pointer to the port on 
 * success or NULL on failure.  'name' can either be the name to give the port
 * or NULL if not needed. */
mb_port *
mb_new_port(const char *name)
{
    mb_port *mport;
    mport = (mb_port *)malloc(sizeof(mb_port));
    if(mport != NULL) {
        initport(mport);
        /* If this fails we'll just ignore it for now since the name isn't really all that important */
        if(name != NULL) {
            mport->name = strdup(name);
        }
    }
    return mport;    
}

/* This function closes the port and frees all the memory associated with it. */
void
mb_destroy_port(mb_port *port) {
    mb_cmd *this;
    
    /* TODO: Close the port */
    
    if(port->name != NULL) free(port->name);
    if(port->device != NULL) free(port->device);
    
    /* destroys all of the commands */
    this = port->commands;
    while(this != NULL) {
        destroy_cmd(this);
        this = this->next;
    }
        
}

/* This function sets the port up as a normal serial port. 'device' is the system device file that represents 
 * the serial port to use.  baudrate is an integer representation of the baudrate, 'parity' can either be
 * MB_NONE, MB_EVEN, or MB_ODD, and 'stopbits' is either 0, 1 or 2. */
int
mb_set_serial_port(mb_port *port, const char *device, int baudrate, short databits, short parity, short stopbits) {
    
    port->devtype = MB_SERIAL;
    port->device = strdup(device);
    if(port->device == NULL) {
        DEBUGMSG("mb_set_serial_port() - Unable to allocate space for port");
        return MB_ERR_ALLOC;
    }
    
    port->baudrate = getbaudrate(baudrate);
    if(port->baudrate == 0) {
        DEBUGMSG("mb_set_serial_port() - Bad baudrate passed");
        return MB_ERR_BAUDRATE;
    }
    if(databits >= 5 && databits <= 8) {
        port->databits = databits;
    } else {
        DEBUGMSG("mb_set_serial_port() - Wrong number of databits passed");
        return MB_ERR_DATABITS;
    }
    if(stopbits == 1 || stopbits == 2) {
        port->stopbits = stopbits;
    } else {
        DEBUGMSG("mb_set_serial_port() - Wrong number of stopbits passed");
        return MB_ERR_STOPBITS;
    }
    return 0;
}

/* This function sets the port up a network port instead of a serial port.  Normally this would be 
 * used for Modbus TCP but with this library it can also be used for the RTU and ASCII protocols
 * as well.  Using a network port for these protocols will allow this library to talk to device 
 * servers over the network. 'ipaddress' is a string representing the ipaddress i.e. "10.10.10.2"
 * 'port' is an integer representing the port to connect too, and 'socket' is either UDP_SOCK or
 * TCP_SOCK.  If the port is a master or client the ipaddress is the server/slave to connect too
 * otherwise it is the address of the local interface to listen on.  port is used similarly. */
int
mb_set_network_port(mb_port *port, const char *ipaddress, unsigned int bindport, unsigned char socket) {
    
    port->devtype = MB_NETWORK;
    
    if(ipaddress == NULL) {
        strcpy(port->ipaddress, "0.0.0.0");
    } else {
        strncpy(port->ipaddress, ipaddress, 15);
    }
    
    port->bindport = bindport;
    if(socket == UDP_SOCK || socket == TCP_SOCK) {
        port->socket = socket;
    } else {
        DEBUGMSG("mb_set_networ_port() - Bad argument for socket");
        return MB_ERR_SOCKET;
    }
    return 0;
}

int
mb_set_protocol(mb_port *port, unsigned char type, unsigned char protocol, u_int8_t slaveid) {
    return 0;
}




void
mb_set_output_callback(mb_port *mp, void (*outfunc)(mb_port *,u_int8_t *,unsigned int))
{
    mp->out_callback = outfunc;
}


void
mb_set_input_callback(mb_port *mp, void (*infunc)(mb_port *,u_int8_t *,unsigned int))
{
    mp->in_callback = infunc;
}


/*********************/
/* Utility Functions */
/*********************/

/* Adds a new command to the linked list of commands on port p 
   This is the master port threads list of commands that it sends
   while running.  If all commands are to be asynchronous then this
   would not be necessary.  Slave ports would never use this.
   Returns 0 on success. */
int
add_cmd(mb_port *p, mb_cmd *mc)
{
    mb_cmd *node;
    
    if(p == NULL || mc == NULL) return -1;
    
    if(p->commands == NULL) {
        p->commands = mc;
    } else {
        node = p->commands;
        while(node->next != NULL) {
            node = node->next;
        }
        node->next = mc;
    }
    return 0;
}
    