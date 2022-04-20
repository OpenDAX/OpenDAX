/* mbports.c - Modbus (tm) Communications Library
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

#include "modbus.h"

extern dax_state *ds;

/* Initializes the port structure given by pointer p */
static void
initport(mb_port *p)
{
    p->name = NULL;
    p->flags = 0x00;
    p->device = NULL;
    p->fd = 0;
    p->enable = 1;
    p->type = 0;
    p->protocol = 0;
    p->devtype = MB_SERIAL;
    p->slaveid = 1;
    p->baudrate = B9600;
    p->databits = 8;
    p->stopbits = 1;
    p->timeout = 1000;
    p->frame = 1000;
    p->delay = 0;
    p->retries = 3;
    p->parity = MB_NONE;
    p->bindport = 5001;
    p->scanrate = 1000;
    p->hold_size = 0;
    p->input_size = 0;
    p->coil_size = 0;
    p->disc_size = 0;
    p->buff_head = NULL;
    FD_ZERO(&(p->fdset));
    p->maxfd = 0;
    p->running = 0;
    p->inhibit = 0;
    p->commands = NULL;
    p->out_callback = NULL;
    p->in_callback = NULL;
    p->slave_read = NULL;
    p->slave_write = NULL;
    strcpy(p->ipaddress, "0.0.0.0");
    p->connections = malloc(sizeof(tcp_connection) * MB_INIT_CONNECTION_SIZE);
    p->connection_size = MB_INIT_CONNECTION_SIZE;
    p->connection_count = 0;
    p->persist = 1;
    pthread_mutex_init(&p->send_lock, NULL);
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
int
openIPport(mb_port *mp, struct in_addr address, uint16_t port)
{
    int fd = 0;
    struct sockaddr_in addr;
    int result;

    if(mp->socket == TCP_SOCK) {
    	fd = socket(AF_INET, SOCK_STREAM, 0);
    } else if (mp->socket == UDP_SOCK) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr = address;
    addr.sin_port = htons(port);

    result = connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    if(result == -1) {
        return -1;
    }
    result = fcntl(fd, F_SETFL, O_NONBLOCK);
    if(result) {
        dax_error(ds, "Unable to set socket to non blocking");
        return -1 ;
    }
    return fd;
}

/* This allocates and initializes a port.  Returns the pointer to the port on
 * success or NULL on failure.  'name' can either be the name to give the port
 * or NULL if not needed. */
mb_port *
mb_new_port(const char *name, unsigned int flags)
{
    mb_port *mport;
    mport = (mb_port *)malloc(sizeof(mb_port));
    if(mport != NULL) {
        initport(mport);
        /* If this fails we'll just ignore it for now since
         * the name isn't really all that important */
        if(name != NULL) {
            mport->name = strdup(name);
            mport->flags = flags;
        }
        if(mport->connections == NULL) {
            dax_error(ds, "Unable to allocate initial connection pool");
            return NULL;
        }
    }
    return mport;
}

/* recursive function that destroys all of the commands */
static void
_free_cmd(mb_cmd *cmd)
{
    if(cmd == NULL) return;
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
        dax_error(ds, "%s - Unable to allocate space for port", __func__);
        return MB_ERR_ALLOC;
    }

    port->baudrate = getbaudrate(baudrate);
    if(port->baudrate == 0) {
        dax_error(ds, "Bad baudrate passed");
        return MB_ERR_BAUDRATE;
    }
    if(databits >= 5 && databits <= 8) {
        port->databits = databits;
    } else {
        dax_error(ds, "Wrong number of databits passed");
        return MB_ERR_DATABITS;
    }
    if(stopbits == 1 || stopbits == 2) {
        port->stopbits = stopbits;
    } else {
        dax_error(ds, "Wrong number of stopbits passed");
        return MB_ERR_STOPBITS;
    }
    return 0;
}

/* This function sets the port up as a network port instead of a serial port.  Normally this would be
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
        dax_error(ds, "Bad argument for socket");
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
mb_set_protocol(mb_port *port, unsigned char type, unsigned char protocol, uint8_t slaveid)
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


const char *mb_get_port_name(mb_port *port) {
    return (const char *)port->name;
}


/* Determines whether or not the port is a serial port or an IP
 * socket and opens it appropriately */
/* TODO This function can be eliminated */
int
mb_open_port(mb_port *m_port)
{
    int fd;

    /* Network connections will happen later */
    if(m_port->devtype == MB_NETWORK) {
        return 0;
    } else {
        fd = openport(m_port);
    }
    if(fd > 0) return 0;
    return fd;
}

int
mb_close_port(mb_port *port)
{
    int result;

    if(port->devtype == MB_NETWORK) {
        for(int n=0; n<port->connection_count; n++) {
            result = close(port->connections[n].fd);
            port->connections[n].addr.s_addr = 0x0000;
            port->connections[n].port = 0;
            port->connections[n].fd = 0;
            if(result) {
                dax_error(ds, "Error closing network file descriptor %d", port->connections[n].fd);
            }
        }
        port->connection_count = 0;
    } else {
         result = close(port->fd);
         port->fd = 0;
         if(result) {
              dax_error(ds, "Error closing file descriptor %d", port->fd);
         }
    }
    return 0;
}


int
mb_set_maxfailures(mb_port *port, int maxfailures, int inhibit)
{
    port->maxattempts = maxfailures;
    port->inhibit_time = inhibit;
    return 0;
}

unsigned char
mb_get_port_protocol(mb_port *port) {
    return port->protocol;
}

int
mb_set_holdreg_size(mb_port *port, unsigned int size) {
    if(size >= 0 && size <=65536) {
        port->hold_size = size;
    } else {
        port->hold_size = 0;
        return MB_ERR_BAD_ARG;
    }
    return 0;
}

int
mb_set_inputreg_size(mb_port *port, unsigned int size) {
    if(size >= 0 && size <=65536) {
        port->input_size = size;
    } else {
        port->input_size = 0;
        return MB_ERR_BAD_ARG;
    }
    return 0;
}

int
mb_set_coil_size(mb_port *port, unsigned int size) {
    if(size >= 0 && size <=65536) {
        port->coil_size = size;
    } else {
        port->coil_size = 0;
        return MB_ERR_BAD_ARG;
    }
    return 0;
}

int
mb_set_discrete_size(mb_port *port, unsigned int size) {
    if(size >= 0 && size <=65536) {
        port->disc_size = size;
    } else {
        port->disc_size = 0;
        return MB_ERR_BAD_ARG;
    }
    return 0;
}


/* This sets the msgout callback function.  The given function will receive the bytes
 * that are actually being sent by the modbus functions. */
/* TODO: I know better than to have callbacks without user data */
void
mb_set_msgout_callback(mb_port *mp, void (*outfunc)(mb_port *port, uint8_t *buff, unsigned int size))
{
    mp->out_callback = outfunc;
}

/* The msgin callback receives the bytes that are coming in from the port */
void
mb_set_msgin_callback(mb_port *mp, void (*infunc)(mb_port *port,uint8_t *buff, unsigned int size))
{
    mp->in_callback = infunc;
}

/*
 * The slave_read_callback function is called by the server loop just before the response
 * is built and returned to the requesting client/master.  This would be used to read
 * data from some external source and put it into the affected register area just before
 * the response is sent.
 */
void
mb_set_slave_read_callback(mb_port *mp, void (*infunc)(struct mb_port *port, int reg, int index, int count, uint16_t *data))
{
    mp->slave_read = infunc;
}

/*
 * The slave_write_callback function is called by the server loop just after the data from
 * the received frame has been written into the register memory.  This function would be
 * used to write the changed data out to an external database.
 */
void
mb_set_slave_write_callback(mb_port *mp, void (*infunc)(struct mb_port *port, int reg, int index, int count, uint16_t *data))
{
    mp->slave_write = infunc;
}

/* returns the next connection in the pool unless we are full then
 * reallocate the pool and double it's size */
static inline int
_get_next_connection(mb_port *mp) {
    tcp_connection *new;

    if(mp->connection_count == mp->connection_size) {
        /* need to grow the connections array */
        DF("Growing connection pool array");
        if(mp->connection_size >= MB_MAX_CONNECTION_SIZE) return ERR_2BIG;
        new = realloc(mp->connections, sizeof(tcp_connection)*mp->connection_size*2);
        if(new == NULL) return MB_ERR_ALLOC;
        else {
            mp->connections = new;
            bzero(&mp->connections[mp->connection_size], sizeof(tcp_connection)*mp->connection_size);
            mp->connection_size *= 2;
        }
    }
    return mp->connection_count++;
}

/* This function retrieves a connection from the ports connection pool
 * if the conneciton does not exist then it attempts to make the connection
 * and stores that in the pool for later.  Returns the file descriptor
 * on success or an error otherwise*/
int
mb_get_connection(mb_port *mp, struct in_addr address, uint16_t port) {
    int n, fd;

    for(n=0;n<mp->connection_count;n++) {
        if(mp->connections[n].addr.s_addr == address.s_addr && mp->connections[n].port == port) {
            /* We found one that matches */
            DF("Found existing connection %d at index %d", mp->connections[n].fd, n)
            return mp->connections[n].fd;
        }
    }
    /* If we get here we didn't find one */
    n = _get_next_connection(mp);
    DF("Opening new connection at index %d", n);
    fd = openIPport(mp, address, port);
    if(fd>=0) {
        mp->connections[n].addr = address;
        mp->connections[n].port = port;
        mp->connections[n].fd = fd;
    }
    DF("Got connection %d\n", fd);
    return fd;
}


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

/* This function is used for debugging purposes.  It could be used
 * to print the port configuration to a file as well. */
void
mb_print_portconfig(FILE *fd, mb_port *mp)
{
    int i;
    mb_cmd *mc;

    fprintf(fd, "Port %s\n", mp->name);
    /* Serial Port Specific Configuration */
    if(mp->devtype == MB_SERIAL) {
        fprintf(fd, "Serial Port: %s %d:%d:", mp->device, mp->baudrate, mp->databits);
        if(mp->parity == MB_EVEN) fprintf(fd, "E:");
        else if(mp->parity == MB_ODD) fprintf(fd, "O:");
        else if(mp->parity == MB_NONE) fprintf(fd, "N:");
        else fprintf(fd, "*:");
        fprintf(fd, "%d\n", mp->stopbits);
    /* Network Port Specific Configuration */
    } else if(mp->devtype == MB_NETWORK) {
        fprintf(fd, "Network Port %s:%d\n", mp->ipaddress, mp->bindport);
    }
    fprintf(fd, "Port Type: ");
    if(mp->protocol == MB_RTU) fprintf(fd, "RTU - ");
    else if(mp->protocol == MB_ASCII) fprintf(fd, "ASCII - ");
    else if(mp->protocol == MB_TCP) fprintf(fd, "TCP - ");
    else fprintf(fd, "* - ");

    if(mp->devtype == MB_SERIAL) {
        if(mp->type == MB_MASTER) fprintf(fd, "Master");
        else if(mp->type == MB_SLAVE) fprintf(fd, "Slave");
        else fprintf(fd, "*");
    } else if(mp->devtype == MB_NETWORK) {
        if(mp->type == MB_CLIENT) fprintf(fd, "Client");
        else if(mp->type == MB_SERVER) fprintf(fd, "Server");
        else fprintf(fd, "*");
    } else {
        fprintf(fd, "Unknown Type");
    }
    fprintf(fd, "\n");
    fprintf(fd, "Intercommand delay: %d mSec\n", mp->delay);
    fprintf(fd, "Interbyte Timeout: %d mSec\n", mp->frame);
    fprintf(fd, "Retries: %d\n", mp->retries);
    fprintf(fd, "Scan Rate: %d mSec\n", mp->scanrate);
    fprintf(fd, "Timeout: %d mSec\n", mp->timeout);
    fprintf(fd, "Max Failures: %d\n", mp->maxattempts);
    fprintf(fd, "Inhibit Time: %d Seconds\n", mp->inhibit_time);
    fprintf(fd, "Persist Connection: %s\n", mp->persist ? "Yes" : "No");

    mc = mp->commands;
    if(mc == NULL) fprintf(fd, "No commands configured for this port\n");
    else {
        i = 0;
        if(mp->protocol == MB_TCP) {
            fprintf(fd, "  Cmd              Addr  Port Node  FC Register Len\n");
            while(mc != NULL) {
                fprintf(fd, " %4d  %15s %5d %4d  %2d %5d   %3d\n",i++,
                                                            inet_ntoa(mc->ip_address),
                                                            mc->port,
                                                            mc->node,
                                                            mc->function,
                                                            mc->m_register,
                                                            mc->length);
                mc = mc->next;
            }
        } else {
            fprintf(fd, "  Cmd  Node  FC Register Len\n");
            while(mc != NULL) {
                fprintf(fd, " %4d  %4d  %2d %5d   %3d\n",i++,mc->node,
                                                      mc->function,
                                                      mc->m_register,
                                                      mc->length);
                mc = mc->next;
            }
        }
    }
    if(mp->type == MB_SLAVE) {
        fprintf(fd, "Slave ID: %d\n", mp->slaveid);
        fprintf(fd, "Coils: %d\n", mp->coil_size);
        fprintf(fd, "Discrete Inputs: %d\n", mp->disc_size);
        fprintf(fd, "Holding Registers: %d\n", mp->hold_size);
        fprintf(fd, "Input Registers: %d\n", mp->hold_size);
    }
    fprintf(fd, "\n");
}
