/* modbus.c - Modbus (tm) Communications Library
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
//#define __USE_XOPEN
#include <unistd.h>
//#undef __USE_XOPEN
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>

#include <modbus.h>
#include <database.h>

static int openport(struct mb_port *);
static int openIPport(struct mb_port *);

void masterRTUthread(struct mb_port *);

/************************
 *  Support functions   *
 ************************/

/* Initializes the port structure given by pointer p */
static void
initport(struct mb_port *p)
{
    p->name = NULL;
    p->device = NULL;
    p->enable = 0;
    p->type = 0; 
    p->protocol = 0;
    p->devtype = SERIAL;
    p->slaveid = 1;      
    p->baudrate = B9600;
    p->databits = 8;
    p->stopbits = 1;
    p->timeout = 1000;      
    p->wait = 10;
    p->delay = 0;     
    p->retries = 3;
    p->parity = NONE;  
    p->bindport = 5001;
    p->scanrate = 1000; 
    p->holdreg = 0;  
    p->holdsize = 0; 
    p->inputreg = 0;
    p->inputsize = 0;
    p->coilreg = 0;
    p->coilsize = 0;
    p->floatreg = 0;
    p->floatsize = 0;
    p->running = 0;
    p->inhibit = 0;
    p->commands = NULL;
    p->out_callback = NULL;
    p->in_callback = NULL;
};

/* Sets the command values to some defaults */
static void
initcmd(struct mb_cmd *c)
{
    c->method = 0;
    c->node = 0;
    c->function = 0;
    c->m_register = 0;
    c->length = 0;
    //--c->address = 0;
    c->handle = 0;
    c->index = 0;
    c->interval = 0;
    
    c->icount = 0;
    c->requests = 0;
    c->responses = 0;
    c->timeouts = 0;
    c->crcerrors = 0;
    c->exceptions = 0;
    c->lasterror = 0;
    c->lastcrc = 0;
    c->firstrun = 0;
    c->next = NULL;
};

/* CRC table straight from the modbus spec */
static unsigned char aCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40
};

/* Table of CRC values for low order byte */
static char aCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
    0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
    0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
    0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
    0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
    0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
    0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
    0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
    0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
    0x40
};


/* Modbus CRC16 checksum calculation. Taken straight from the modbus specification,
   but I fixed the typos and changed the name to protect the guilty */ 
unsigned short
crc16(unsigned char *msg, unsigned short length)
{
    unsigned char CRCHi = 0xFF;
    unsigned char CRCLo = 0xFF;
    unsigned int index;
    while(length--) {
        index = CRCHi ^ *msg++;
        CRCHi = CRCLo ^ aCRCHi[index];
        CRCLo = aCRCLo[index];
    }
    return (CRCHi << 8 | CRCLo);
};

/* Checks the checksum of the modbus message given by *buff 
   length should be the length of the modbus data buffer INCLUDING the
   two byte checksum.  Returns 1 if checksum matches */
static int
crc16check(u_int8_t *buff,int length)
{
    u_int16_t crcLocal, crcRemote;
    if(length < 2) return 0; /* Otherwise a bad pointer will go to crc16 */
    crcLocal = crc16(buff, length-2);
    COPYWORD(&crcRemote, &buff[length-2]);
    if(crcLocal == crcRemote) return 1;
    else return 0;
};

/* Calculates the difference between the two times */
inline unsigned long long
timediff(struct timeval oldtime,struct timeval newtime)
{
    return (newtime.tv_sec - oldtime.tv_sec) * 1000 + 
           (newtime.tv_usec / 1000)-(oldtime.tv_usec / 1000);
};


/************************
 *   Public functions   *
 ************************/

/* Create the datatable and allocate the ports array */
/* GONNA GO WITHOUT THESE FOR NOW***********

int mb_init(unsigned int port_count, u_int16_t dt_size) {
    int result,n;
    
    result=dt_init(dt_size);
    if(result) return result;
    
    _ports=(struct mb_port *)malloc(sizeof(struct mb_port)*port_count);
    if(_ports==NULL) return -1;
    
    for(n=0;n<port_count;n++) {
        initport(&_ports[n]);
    }
    _port_count=port_count;
    
    
    return 0;
}*/

/* Destroy the whole thing. Unimplemented at present*/
/*
void mb_destroy(void) {
    return;
}*/

/* This allocates and initializes a port.  Returns the pointer to the port on 
   success or NULL on failure. */
struct mb_port *
mb_new_port(void)
{
    struct mb_port *mport;
    mport=(struct mb_port *)malloc(sizeof(struct mb_port));
    if(mport != NULL) {
        initport(mport);
    }
    return mport;    
}

/* Allocates and initializes a master modbus command.  Returns the pointer
   to the new command or NULL on failure. */
struct mb_cmd *
mb_new_cmd(void)
{
    struct mb_cmd *mcmd;
    mcmd = (struct mb_cmd *)malloc(sizeof(struct mb_cmd));
    if(mcmd != NULL) {
        initcmd(mcmd);
    }
    return mcmd;    
}

void
mb_set_output_callback(struct mb_port *mp, void (* outfunc)(struct mb_port *,u_int8_t *,unsigned int))
{
    mp->out_callback = outfunc;
}


void
mb_set_input_callback(struct mb_port *mp, void (* infunc)(struct mb_port *,u_int8_t *,unsigned int))
{
    mp->in_callback = infunc;
}

/* Adds a new command to the linked list of commands on port p 
   This is the master port threads list of commands that it sends
   while running.  If all commands are to be asynchronous then this
   would not be necessary.  Slave ports would never use this.
   Returns 0 on success. */
int
mb_add_cmd(struct mb_port *p, struct mb_cmd *mc)
{
    struct mb_cmd *node;
    
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

/* Finds the modbus master command indexed by cmd.  Returns a pointer
   to the command when found and NULL on error. */
struct
mb_cmd *mb_get_cmd(struct mb_port *mp,unsigned int cmd)
{
    struct mb_cmd *node;
    unsigned int n = 0;
    if(mp == NULL) return NULL;
    
    if(mp->commands == NULL) {
        node = NULL;
    } else { 
        node = mp->commands;
        do {
            if(n++ == cmd) return node;
            node = node->next;
        } while(node != NULL);
    }
    return node;
}

/* Determines whether or not the port is a serial port or an IP
   socket and opens it appropriately */
int
mb_open_port(struct mb_port *m_port)
{
    int fd;
    
    /* if the LAN bit or the TCP bit are set then we need a network
        socket.  Otherwise we need a serial port opened */
    if(m_port->devtype == NET) {
        fd = openIPport(m_port);
    } else {
        fd = openport(m_port);
    }
    if(pthread_mutex_init (&m_port->port_mutex, NULL)) {
        dax_error("Problem Initilizing Mutex for port: %s", m_port->name);
        return -1;
    }
    if(fd > 0) return 0;
    return fd;
}

/* Opens the port passed in m_port and starts the thread that
   will handle the port */
int
mb_start_port(struct mb_port *m_port)
{
    pthread_attr_t attr;
    
    /* If the port is not already open */
    if(!m_port->fd) {
        if(mb_open_port(m_port)) {
            dax_error( "Unable to open port - %s", m_port->name);
            return -1;
        }
    }
    if(m_port->fd > 0) {
        /* Set up the thread attributes 
         * The thread is set up to be detached so that it's resources will
         * be deallocated automatically. */
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if(m_port->type == MASTER) {
            if(pthread_create(&m_port->thread, &attr, (void *)&masterRTUthread, (void *)m_port)) {
                dax_error( "Unable to start thread for port - %s", m_port->name);
                return -1;
            } else {
                dax_debug(3, "Started Thread for port - %s", m_port->name);
                return 0;
            }
        } else if(m_port->type == SLAVE) {
            ; /* TODO: start slave thread */
        } else {
            dax_error( "Unknown Port Type %d, on port %s", m_port->type, m_port->name);
            return -1;
        }
        return 0;
    } else {
        dax_error( "Unable to open IP Port: %s [%s:%d]", m_port->name, m_port->ipaddress, m_port->bindport);
        return -1;
    }
    return 0; //should never get here
}


/* Open and set up the serial port */
static int
openport(struct mb_port *m_port)
{
    int fd;
    struct termios options;
    
    /* the port is opened RW and reads will not block */
    fd = open(m_port->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd == -1)  {
        dax_error("openport: %s", strerror(errno));
        return(-1);
    } else  {
    
    /* TODO: this should set the port up like the configuration says */
        fcntl(fd, F_SETFL, 0);
        tcgetattr(fd, &options);
        cfsetispeed(&options, m_port->baudrate);
        cfsetospeed(&options, m_port->baudrate);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
      
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
        options.c_oflag &= ~OPOST;
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 0; /* 1 sec */
        /* TODO: Should check for errors here */
        tcsetattr(fd, TCSANOW, &options);
    } 
    m_port->fd = fd;
    return fd;
}

/* Opens a IP socket instead of a serial port for both
   the TCP protocol and the LAN protocol. */
static int
openIPport(struct mb_port *mp)
{
    int fd;
    struct sockaddr_in addr;
    int result;
    
    dax_debug(10, "Opening IP Port");
    if(mp->socket == TCP_SOCK) {
		fd = socket(AF_INET, SOCK_STREAM, 0);
	} else if (mp->socket == UDP_SOCK) {
		fd = socket(AF_INET, SOCK_DGRAM, 0);
	} else {
        dax_error( "Unknown socket type");
	    return -1;
	}
	
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(mp->ipaddress);
    addr.sin_port = htons(mp->bindport);
    
    result = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    dax_debug(10, "Connect returned %d", result);
    if(result == -1) {
        dax_error( "openIPport: %s", strerror(errno));
        return -1;
    }
    result = fcntl(fd, F_SETFL, O_NONBLOCK);
    if(result) {
        dax_error( "Unable to set socket to non blocking");
        return -1 ;
    }
    dax_debug(10, "Socket Connected, fd = %d", fd);
    mp->fd = fd;
    return fd;
}
        

/* This is the primary thread for a Modbus RTU master.  It calls the functions
   to send the request and recieve the responses.  It also takes care of the
   retries and the counters. */
/* TODO: There needs to be a mechanism to bail out of here.  The mp->running should
be set to 0 and the mp->fd closed.  This will cause the main while(1) loop to restart
the port */
void
masterRTUthread(struct mb_port *mp)
{
    long time_spent;
    struct mb_cmd *mc;
    struct timeval start, end;
    unsigned char bail = 0;
    
    mp->running = 1; /* Tells the world that we are going */
    mp->attempt = 0;
    /* If enable goes negative we bail at the next scan */
    while(mp->enable >= 0 && !bail) {
        gettimeofday(&start, NULL);
        if(mp->enable) { /* If enable=0 then pause for the scanrate and try again. */
            mc = mp->commands;
            while(mc != NULL && !bail) {
                /* Only if the command is enabled and the interval counter is over */
                if(mc->method && (++mc->icount >= mc->interval)) { 
                    mc->icount = 0;
                    if(mp->maxattempts) {
                        mp->attempt++;
                        DAX_DEBUG2("Incrementing attempt - %d", mp->attempt);
                    }
                    if( mb_send_command(mp, mc) > 0 )
                        mp->attempt = 0; /* Good response, reset counter */
                    
                    if((mp->maxattempts && mp->attempt >= mp->maxattempts) || mp->dienow) {
                        bail = 1;
                        mp->inhibit_temp = 0;
                        mp->inhibit = 1;
                    }
                }
                if(mp->delay > 0) usleep(mp->delay * 1000);
                mc = mc->next; /* get next command from the linked list */
            } /* End of while for sending commands */
        }
        /* This calculates the length of time that it took to send the messages on this port
           and then subtracts that time from the port's scanrate and calls usleep to hold
           for the right amount of time.  */
        gettimeofday(&end, NULL);
        time_spent = (end.tv_sec-start.tv_sec)*1000 + (end.tv_usec/1000 - start.tv_usec/1000);
        /* If it takes longer than the scanrate then just go again instead of sleeping */
        if(time_spent < mp->scanrate)
            usleep((mp->scanrate - time_spent) * 1000);
    }
     /* Close the port */
    if(close(mp->fd)) {
        dax_error("Could not close port");
    }
    dax_error("Too Many Errors: Shutting down port - %s", mp->name);
    mp->fd = 0;
    mp->dienow = 0;
    mp->running = 0;
}

/* This function formulates and sends the modbus master request */
static int
sendRTUrequest(struct mb_port *mp,struct mb_cmd *cmd)
{
    u_int8_t buff[MB_FRAME_LEN], length;
    u_int16_t crc, temp, result;
    
    /* build the request message */
    buff[0]=cmd->node;
    buff[1]=cmd->function;
    
    switch (cmd->function) {
        case 1:
        case 3:
        case 4:
            COPYWORD(&buff[2], &cmd->m_register);
            COPYWORD(&buff[4], &cmd->length);
            length = 6;
            break;
        case 5:
            //--temp = dt_getbit(cmd->handle, cmd->index);
            temp = 0;
            result = dt_getbits(cmd->handle, cmd->index, &temp, 1);
            //--printf("...getbits returned 0x%X for index %d\n", temp, cmd->index);
            /* TODO: Error Check result */
            if(cmd->method == MB_CONTINUOUS || (temp != cmd->lastcrc) || !cmd->firstrun ) {
                COPYWORD(&buff[2], &cmd->m_register);
                if(temp) buff[4] = 0xff;
                else     buff[4] = 0x00;
                buff[5] = 0x00;
                cmd->firstrun = 1;
                cmd->lastcrc = temp;
                length = 6;
                break;
            } else {
                return 0;
            }
            break;
        case 6:
            //--temp = dt_getword(cmd->address);
            result = dt_getwords(cmd->handle, cmd->index, &temp, 1);
            /* TODO: Error Check result */
            /* If the command is contiunous go, if conditional then 
             check the last checksum against the current datatable[] */
            if(cmd->method == MB_CONTINUOUS || (temp != cmd->lastcrc)) {
                COPYWORD(&buff[2], &cmd->m_register);
                COPYWORD(&buff[4], &temp);
                cmd->lastcrc = temp; /* Since it's a single just store the word */
                length = 6;
                break;
            } else {
                return 0;
            }
            default:
            break;
    }
    crc = crc16(buff,length);
    COPYWORD(&buff[length],&crc);
    /* Send Request */
    cmd->requests++; /* Increment the request counter */
    tcflush(mp->fd, TCIOFLUSH);
    /* Send the buffer to the callback routine. */
    if(mp->out_callback) {
        mp->out_callback(mp, buff, length + 2);
    }
    
    return write(mp->fd, buff, length + 2);
}

/* This function waits for one interbyte timeout (wait) period and then
 checks to see if there is data on the port.  If there is no data and
 we still have not received any data then it compares the current time
 against the time the loop was started to see about the timeout.  If there
 is data then it is written into the buffer.  If there is no data on the
 read() and we have received some data already then the full message should
 have been received and the function exits. 
 
 Returns 0 on timeout
 Returns -1 on CRC fail
 Returns the length of the message on success */
static int
getRTUresponse(u_int8_t *buff, struct mb_port *mp)
{
    unsigned int buffptr = 0;
    struct timeval oldtime, thistime;
    int result;
    
    gettimeofday(&oldtime, NULL);
    
    while(1) {
        usleep(mp->wait * 1000);
        result = read(mp->fd, &buff[buffptr], MB_FRAME_LEN);
        if(result > 0) { /* Get some data */
            buffptr += result; // TODO: WE NEED A BOUNDS CHECK HERE.  Seg Fault Commin'
        } else { /* Message is finished, good or bad */
            if(buffptr > 0) { /* Is there any data in buffer? */
                
                if(mp->in_callback) {
                    mp->in_callback(mp, buff, buffptr);
                }
                /* Check the checksum here. */
                result = crc16check(buff, buffptr);
                
                if(!result) return -1;
                else return buffptr;
                
            } else { /* No data in the buffer */
                gettimeofday(&thistime,NULL); 
                if(timediff(oldtime, thistime) > mp->timeout) {
                    return 0;
                }
            }
        }
    }
    return 0; /* Should never get here */
}

static int
sendASCIIrequest(struct mb_port *mp,struct mb_cmd *cmd)
{
    return 0;
}

static int
getASCIIresponse(u_int8_t *buff,struct mb_port *mp)
{
    return 0;
}

/* This function takes the message buffer and the current command and
 determines what to do with the message.  It may write data to the 
 datatable or just return if the message is an acknowledge of a write.
 This function is protocol indifferent so the checksums should be
 checked before getting here.  This function assumes that *buff looks
 like an RTU message so the ASCII response functions should translate
 the ASCII responses into RTUish messages */

static int
handleresponse(u_int8_t *buff, struct mb_cmd *cmd)
{
    int n, result;
    //u_int16_t temp;
    u_int16_t *data;
    
    DAX_DEBUG2("handleresponse() - Function Code = %d", cmd->function);
    cmd->responses++;
    if(buff[1] >= 0x80) 
        return buff[2];
    if(buff[0] != cmd->node)
        return ME_WRONG_DEVICE;
    if(buff[1] != cmd->function)
        return ME_WRONG_FUNCTION;
    /* If we get this far the message should be good */
    switch (cmd->function) {
        case 1:
            //--dt_setbits(cmd->address, &buff[3], cmd->length);
            result = dt_setbits(cmd->handle, cmd->index, &buff[3], cmd->length);
            break;
        case 3:
        case 4:
            data = alloca(buff[2] / 2);
            for(n = 0; n < (buff[2] / 2); n++) {
                COPYWORD(&data[n], &buff[(n * 2) + 3]);
            }
            //--if(dt_setword(cmd->address + n, temp)) return -1;
            /* TODO: There may be times when we get more data than cmd->length but how to deal with that? */
            if(dt_setwords(cmd->handle, cmd->index, data, cmd->length)) return -1;
            break;
        case 5:
        case 6:
            //COPYWORD(&temp, &buff[2]);
            //if(dt_setword(cmd->address,temp)) return -1;
            break;
        default:
            break;
    }
    return 0;
}


/* External function to send a Modbus commaond (mc) to port (mp).  The function
   sets some function pointers to the functions that handle the port protocol and
   then uses those functions generically.  The retry loop tries the command for the
   configured number of times and if successful returns 0.  If not, an error code
   is returned. mb_buff should be at least MB_FRAME_LEN in length */

int
mb_send_command(struct mb_port *mp, struct mb_cmd *mc)
{
    u_int8_t buff[MB_FRAME_LEN]; /* Modbus Frame buffer */
    int try = 1;
    int result, msglen;
    static int (*sendrequest)(struct mb_port *, struct mb_cmd *) = NULL;
    static int (*getresponse)(u_int8_t *,struct mb_port *) = NULL;
    
    
    /*This sets up the function pointers so we don't have to constantly check
      which protocol we are using for communication.  From this point on the 
      code is generic for RTU or ASCII */
    if(mp->protocol == RTU) {
        sendrequest = sendRTUrequest;
        getresponse = getRTUresponse;
    } else if(mp->protocol == ASCII) {
        sendrequest = sendASCIIrequest;
        getresponse = getASCIIresponse;
    } else {
        return -1;
    }
    
    do { /* retry loop */
        pthread_mutex_lock(&mp->port_mutex); /* Lock the port */
                
		result = sendrequest(mp, mc);
		if(result > 0) {
			msglen = getresponse(buff, mp);
		    pthread_mutex_unlock(&mp->port_mutex);
        } else if(result == 0) {
            /* Should be 0 when a conditional command simply doesn't run */
            pthread_mutex_unlock(&mp->port_mutex);
            return result;
        } else {
            pthread_mutex_unlock(&mp->port_mutex);
            return -1;
        }
        
        if(msglen > 0) {
            result = handleresponse(buff,mc);
            if(result > 0) {
                mc->exceptions++;
                mc->lasterror = result | ME_EXCEPTION;
                DAX_DEBUG2("Exception Received - %d", result);
            } else { /* Everything is good */
                mc->lasterror = 0;
            }
            return 0; /* We got some kind of message so no sense in retrying */
        } else if(msglen == 0) {
            DAX_DEBUG("Timeout");
			mc->timeouts++;
			mc->lasterror = ME_TIMEOUT;
        } else {
            /* Checksum failed in response */
            DAX_DEBUG("Checksum");
			mc->crcerrors++;
            mc->lasterror = ME_CHECKSUM;
        }
    } while(try++ <= mp->retries);
    /* After all the retries get out with error */
    /* TODO: Should set error code?? */
    return -2;
}



