/* modbus.h - Modbus (tm) Communications Library
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

#ifndef __MODBUS_H
#define __MODBUS_H

#include <common.h>
#ifdef HAVE_SYS_SOCKET_H
 #include <sys/socket.h>
#endif
#ifdef HAVE_SYS_PARAM_H
 #include <sys/param.h>
#endif
#include <database.h>


#ifdef __BIG_ENDIAN__ /* Mac OSX Endianness */
# define __MB_BIG_ENDIAN
#endif

#ifdef __BYTE_ORDER /* Linux Endianness */
# if __BYTE_ORDER == __BIG_ENDIAN
#   define __MB_BIG_ENDIAN
# endif
#endif

/* Used to insert a 16bit value into the modbus buffer 
 another endian machine would simply use memcpy() */
#ifndef __MB_BIG_ENDIAN
# define COPYWORD(OUT,IN) swab((const void *)IN,(void *)OUT,(ssize_t)2)
#else
# define COPYWORD(OUT,IN) memcpy(OUT,IN,2);
#endif

#define MB_FRAME_LEN 255

#define ALIAS_LENGTH 33
#define ALIAS_COUNT 64
/* Device Port Types */
#define SERIAL 0
#define NET 1
/* Port Types */
#define MASTER 0
#define SLAVE 1
/* Protocols */
#define RTU 1
#define ASCII 2
#define TCP 4
#define LAN 8 /* OR this with RTU or ASCII to get TCP/IP socket passthru */
/* Socket type for network ports */
#define UDP_SOCK 1
#define TCP_SOCK 2

/* Parity */
#define NONE 0
#define EVEN 1
#define ODD 2

/* Modbus Errors */
#define ME_EXCEPTION 0x80;
#define ME_WRONG_DEVICE 1;
#define ME_WRONG_FUNCTION 2;
#define ME_CHECKSUM 4;
#define ME_TIMEOUT 8;

/* I really should make the port and the commands invisible to the outside
   world and use functions to manipulate the members.  For now I'll just 
   get it working. */

struct mb_port {
    char *name;
    char *device;
    char enable;    /* 0=Pause, 1=Run */
    unsigned char type;      /* 0=Master, 1=Slave */
    unsigned char devtype;   /* 0=serial, 1=network */
    unsigned char protocol;  /* [Only RTU is implemented so far] */
    u_int8_t slaveid;        /* Slave ID 1-247 (Slave Only) */
    unsigned int baudrate;
    unsigned char databits;
    unsigned char stopbits;
    unsigned char parity;    /* 0=NONE, 1=EVEN OR 2=ODD */
    char ipaddress[16];
    unsigned int bindport;   /* IP port to bind to */
    unsigned char socket;    /* either UDP_SOCK or TCP_SOCK */
    unsigned int delay;      /* Intercommand delay */
    unsigned int wait;       /* Interbyte timeout */
    unsigned short retries;  /* Number of retries to try */
    unsigned int scanrate;   /* scanrate in mSeconds */
    unsigned int holdreg;    /* database index for holding registers (slave only) */
    unsigned int holdsize;   /* size of the internaln holding register bank */
    unsigned int inputreg;   /* database index for input registers (slave only) */
    unsigned int inputsize;  /* size of the internal input register bank */
    unsigned int coilreg;    /* database index for coils (slave only) */
    unsigned int coilsize;   /* size of the internal bank of coils in 16-bit registers */
    unsigned int floatreg;       /* database index for Enron style float table */
    unsigned int floatsize;      /* size of the float table */
    unsigned int timeout;        /* Response timeout */
    unsigned int maxtimeouts;    /* Number of timeouts to allow before closing and exiting the port */
    struct mb_cmd *commands;     /* Linked list of Modbus commands */
    int fd;                      /* File descriptor to the port */
    pthread_mutex_t port_mutex;  /* Port Mutex */
    unsigned char running;       /* Flag to indicate the port is running */
    unsigned char inhibit;       /* When set the port will not be started */
    unsigned int inhibit_time;   /* Number of seconds before the port will be retried */
    unsigned int inhibit_temp;
    /* These are callback function pointers for the port data */
    void (* out_callback)(struct mb_port *, u_int8_t *, unsigned int);
    void (* in_callback)(struct mb_port *, u_int8_t *, unsigned int);
};

/* Command Methods */
#define	MB_DISABLE 0
#define MB_CONTINUOUS 1
#define	MB_ONCHANGE 2

struct mb_cmd {
    unsigned char method;    /* 0=disable 1=continuous 2=on change */
    u_int8_t node;           /* Modbus device ID */
    u_int8_t function;       /* Function Code */
    u_int16_t m_register;    /* Modbus Register */
    u_int16_t length;        /* length of modbus data */
    unsigned int address;    /* datatable address */
    unsigned int interval;   /* number of port scans between messages */
    unsigned int icount;     /* number of intervals passed */
    unsigned int requests;   /* total number of times this command has been sent */
    unsigned int responses;  /* number of valid modbus responses (exceptions included) */
    unsigned int timeouts;   /* number of times this command has timed out */
    unsigned int crcerrors;  /* number of checksum errors */
    unsigned int exceptions; /* number of modbus exceptions recieved from slave */
    u_int8_t lasterror;      /* last error on command */
    u_int16_t lastcrc;       /* used to determine if a conditional message should be sent */
    unsigned char firstrun;  /* Indicates that this command has been sent once */
    struct mb_cmd* next;
};


/* An alias is used to give a datatable point a nickname */
struct mb_alias {
    char name[ALIAS_LENGTH];
    int m_address;
};

struct mb_port *mb_new_port(void);
struct mb_cmd *mb_new_cmd(void);
void mb_set_output_callback(struct mb_port *, void (* outfunc)(struct mb_port *,u_int8_t *,unsigned int));
void mb_set_input_callback(struct mb_port *, void (* infunc)(struct mb_port *,u_int8_t *,unsigned int));
int mb_add_cmd(struct mb_port *,struct mb_cmd *);
struct mb_cmd *mb_get_cmd(struct mb_port *,unsigned int);
int mb_open_port(struct mb_port *);
int mb_start_port(struct mb_port *);
int mb_send_command(struct mb_port *, struct mb_cmd *);


#endif
