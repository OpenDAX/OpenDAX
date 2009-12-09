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
#include <mblib.h>

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
    p->frame = 10;
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
    p->discreg = NULL;
    p->discsize = 0;
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
        case 57600:
            return B57600;
#ifdef B76800 /* Not a common baudrate */
        case 76800:
            return B76800;
#endif
        case 115200:
            return B115200;
        default:
            return 0;
    }
}

/* Open and set up the serial port */
static int
openport(mb_port *m_port)
{
    int fd;
    struct termios options;
    
    /* the port is opened RW and reads will not block */
    fd = open(m_port->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd == -1)  {
        DEBUGMSG2("openport: %s", strerror(errno));
        return(-1);
    } else  {
        fcntl(fd, F_SETFL, 0);
        tcgetattr(fd, &options);
        /* Set the baudrate */
        cfsetispeed(&options, m_port->baudrate);
        cfsetospeed(&options, m_port->baudrate);
        options.c_cflag |= (CLOCAL | CREAD);
        /* Set the parity */
        if(m_port->parity == MB_ODD) {
            options.c_cflag |= PARENB;        
            options.c_cflag |= PARODD;        
        } else if(m_port->parity == MB_EVEN) {
            options.c_cflag |= PARENB;        
            options.c_cflag &= ~PARODD;        
        } else { /* No Parity */ 
            options.c_cflag &= ~PARENB;
        }
        /* Set stop bits */
        if(m_port->stopbits == 2) {
            options.c_cflag |= CSTOPB;
        } else {
            options.c_cflag &= ~CSTOPB;
        }
        /* Set databits */
        options.c_cflag &= ~CSIZE;
        if(m_port->databits == 5) {
            options.c_cflag |= CS5;    
        } else if(m_port->databits == 6) {
            options.c_cflag |= CS6;
        } else if(m_port->databits == 7) {
            options.c_cflag |= CS7;
        } else {
            options.c_cflag |= CS8;    
        }
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
        options.c_oflag &= ~OPOST;
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 0;
        /* TODO: Should check for errors here */
        tcsetattr(fd, TCSANOW, &options);
    } 
    m_port->fd = fd;
    return fd;
}

/* Opens a IP socket instead of a serial port for both
   the TCP protocol and the LAN protocol. */
static int
openIPport(mb_port *mp)
{
    int fd;
    struct sockaddr_in addr;
    int result;
    
    DEBUGMSG("Opening IP Port");
    if(mp->socket == TCP_SOCK) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
    } else if (mp->socket == UDP_SOCK) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(mp->ipaddress);
    addr.sin_port = htons(mp->bindport);
    
    result = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    DEBUGMSG2("Connect returned %d", result);
    if(result == -1) {
        DEBUGMSG2( "openIPport: %s", strerror(errno));
        return -1;
    }
    result = fcntl(fd, F_SETFL, O_NONBLOCK);
    if(result) {
        DEBUGMSG( "Unable to set socket to non blocking");
        return -1 ;
    }
    DEBUGMSG2( "Socket Connected, fd = %d", fd);
    mp->fd = fd;
    return fd;
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
        /* If this fails we'll just ignore it for now since
         * the name isn't really all that important */
        if(name != NULL) {
            mport->name = strdup(name);
        }
    }
    return mport;    
}

/* recursive function that destroys all of the commands */
static void
_free_cmd(mb_cmd *cmd)
{
    if(cmd->next != NULL) {
        _free_cmd(cmd->next);
    }
    mb_destroy_cmd(cmd);
}

/* This function closes the port and frees all the memory associated with it. */
void
mb_destroy_port(mb_port *port)
{
    mb_close_port(port);
    
    if(port->name != NULL) free(port->name);
    if(port->device != NULL) free(port->device);
    
    /* destroys all of the commands */
    _free_cmd(port->commands);
}

/* This function sets the port up as a normal serial port. 'device' is the system device file that represents 
 * the serial port to use.  baudrate is an integer representation of the baudrate, 'parity' can either be
 * MB_NONE, MB_EVEN, or MB_ODD, and 'stopbits' is either 0, 1 or 2. */
int
mb_set_serial_port(mb_port *port, const char *device, int baudrate, short databits, short parity, short stopbits)
{
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
mb_set_network_port(mb_port *port, const char *ipaddress, unsigned int bindport, unsigned char socket)
{
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

/* This function sets up the modbus protocol.  Type is either MB_MASTER or MB_SLAVE
 * (MB_SERVER and MB_CLIENT are also defined but they are the same).  Protocol is
 * either MB_RTU, MB_ASCII or MB_TCP.  'slaveid' is the node id that the port will
 * use if it is configured as a slave.  It will be ignored if the port is type is
 * set up as a modbus master */
int
mb_set_protocol(mb_port *port, unsigned char type, unsigned char protocol, u_int8_t slaveid)
{
    if(type == MB_MASTER || type == MB_SLAVE) {
        port->type = type;
    } else {
        return MB_ERR_PORTTYPE;
    }
    if(protocol == MB_RTU || protocol == MB_ASCII || protocol == MB_TCP) {
        port->protocol = protocol;
    } else {
        return MB_ERR_PROTOCOL;
    }
    port->slaveid = slaveid;
    return 0;
}

const char *mb_get_name(mb_port *port) {
    return (const char *)port->name;
}


/* Determines whether or not the port is a serial port or an IP
 * socket and opens it appropriately */
int
mb_open_port(mb_port *m_port)
{
    int fd;
    
    if(m_port->devtype == MB_NETWORK) {
        fd = openIPport(m_port);
    } else {
        fd = openport(m_port);
    }
    //--Removing pthread from the modbus library??
    //--if(pthread_mutex_init (&m_port->port_mutex, NULL)) {
    //--    dax_error("Problem Initilizing Mutex for port: %s", m_port->name);
    //--    return -1;
    //--}
    if(fd > 0) return 0;
    return fd;
}

int
mb_close_port(mb_port *port)
{
    int result;
    
    result = close(port->fd);
    port->fd = 0;
    return result;
}

int
mb_set_frame_time(mb_port *port, int frame)
{
    port->frame = frame;
    return 0;
}

int
mb_set_delay_time(mb_port *port, int delay)
{
    port->delay = delay;
    return 0;
}

int
mb_set_scan_rate(mb_port *port, int rate)
{
    port->scanrate = rate;
    return 0;    
}

int
mb_set_timeout(mb_port *port, int timeout)
{
    port->timeout = timeout;
    return 0;    
}

int
mb_set_retries(mb_port *port, int retries)
{
    port->retries = retries;
    return 0;    
}

unsigned char
mb_get_type(mb_port *port)
{
    DEBUGMSG2("mb_get_type() called for modbus port %s", port->name);
    DEBUGMSG2("mb_get_type() returning %d", port->type);
    return port->type;
}


/* The following functions are used to set up the data for the slave port */
int
mb_set_register_size(mb_port *port, int reg, int size)
{
    if(reg == MB_REG_HOLDING) {
        //set holding register size
        return 0;
    } else if(reg == MB_REG_INPUT) {
        return 0;
    } else if(reg == MB_REG_COIL) {
        return 0;
    }
    return MB_ERR_GENERIC;
}


/* This sets the msgout callback function.  The given function will receive the bytes
 * that are actually being sent by the modbus functions. */
/* TODO: I know better than to have callbacks without user data */
void
mb_set_msgout_callback(mb_port *mp, void (*outfunc)(mb_port *,u_int8_t *,unsigned int))
{
    mp->out_callback = outfunc;
}

/* The msgin callback receives the bytes that are coming in from the port */
void
mb_set_msgin_callback(mb_port *mp, void (*infunc)(mb_port *,u_int8_t *,unsigned int))
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
    
