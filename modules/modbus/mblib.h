/* modlib.h - Modbus (tm) Communications Library
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
 * This is the private header file for the modbus library functions
 */
 
#ifndef __MODLIB_H
#define __MODLIB_H

/* Are we using config.h */
#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
 #include <sys/socket.h>
#endif
#ifdef HAVE_SYS_PARAM_H
 #include <sys/param.h>
#endif
#ifdef HAVE_STRING_H
 #include <string.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <modbus.h>
//--Do I really want to have this dependency???
//--#include <pthread.h>

//--#include <database.h>

#ifdef __BIG_ENDIAN__ /* PPC Endianness */
# define __MB_BIG_ENDIAN
#endif

#ifdef __BYTE_ORDER /* x86 Endianness */
# if __BYTE_ORDER == __BIG_ENDIAN
#   define __MB_BIG_ENDIAN
# endif
#endif

/* Used to insert a 16bit value into the modbus buffer 
 another endian machine would simply use memcpy() */
#ifndef __MB_BIG_ENDIAN
# define COPYWORD(OUT,IN) swab((const void *)(IN),(void *)(OUT),(ssize_t)2)
#else
# define COPYWORD(OUT,IN) memcpy((OUT),(IN),2);
#endif

#define MB_FRAME_LEN 255

#ifndef MAX_RETRIES
#  define MAX_RETRIES 100
#endif

/* Device Port Types */
#define MB_SERIAL  0
#define MB_NETWORK 1
/* Extra Protocol bit /// May not be needed */
//#define MB_LAN   8 /* OR this with RTU or ASCII to get TCP/IP socket passthru */

//#define ALIAS_LENGTH 33
//#define ALIAS_COUNT 64

struct mb_port {
    char *name;               /* Port name if needed : Maybe we don't need this */
    char *device;             /* device filename of the serial port */
    unsigned char enable;     /* 0=Pause, 1=Run */
    unsigned char type;       /* 0=Master, 1=Slave */
    unsigned char devtype;    /* 0=serial, 1=network */
    unsigned char protocol;   /* [Only RTU is implemented so far] */
    u_int8_t slaveid;         /* Slave ID 1-247 (Slave Only) */
    int baudrate;
    short databits;
    short stopbits;
    short parity;             /* 0=NONE, 1=EVEN OR 2=ODD */
    char ipaddress[16];
    unsigned int bindport;    /* IP port to bind to */
    unsigned char socket;     /* either UDP_SOCK or TCP_SOCK */
    
    int delay;       /* Intercommand delay */
    int frame;       /* Interbyte timeout */
    int retries;     /* Number of retries to try */
    int scanrate;    /* Scanrate in mSeconds */
    int timeout;     /* Response timeout */
    int maxattempts; /* Number of failed attempts to allow before closing and exiting the port */
    
    u_int16_t *holdreg;       /* database index for holding registers (slave only) */
    unsigned int holdsize;    /* size of the internaln holding register bank */
    u_int16_t *inputreg;      /* database index for input registers (slave only) */
    unsigned int inputsize;   /* size of the internal input register bank */
    u_int16_t *coilreg;       /* database index for coils (slave only) */
    unsigned int coilsize;    /* size of the internal bank of coils in 16-bit registers */
    u_int16_t *discreg;       /* discrete input register */
    unsigned int discsize;    /* size of the internal bank of coils */
    
    struct mb_cmd *commands;  /* Linked list of Modbus commands */
    int fd;                   /* File descriptor to the port */
    int dienow;
    //--pthread_t thread;
    //--pthread_mutex_t port_mutex;  /* Port Mutex */
    unsigned int attempt;        /* Attempt counter */
    unsigned char running;       /* Flag to indicate the port is running */
    unsigned char inhibit;       /* When set the port will not be started */
    unsigned int inhibit_time;   /* Number of seconds before the port will be retried */
    unsigned int inhibit_temp;
    /* These are callback function pointers for the port message data */
    void (*out_callback)(struct mb_port *, u_int8_t *, unsigned int);
    void (*in_callback)(struct mb_port *, u_int8_t *, unsigned int);
};

struct mb_cmd {
    unsigned char enable;    /* 0=disable, 1=enable */
    unsigned char mode;      /* MB_CONTINUOUS, MB_ONCHANGE */
    u_int8_t node;           /* Modbus device ID */
    u_int8_t function;       /* Function Code */
    u_int16_t m_register;    /* Modbus Register */
    u_int16_t length;        /* length of modbus data */
    unsigned int interval;   /* number of port scans between messages */
    u_int8_t *data;          /* pointer to the actual modbus data that this command refers */
    int datasize;            /* size of the *data memory area */
    unsigned int icount;     /* number of intervals passed */
    unsigned int requests;   /* total number of times this command has been sent */
    unsigned int responses;  /* number of valid modbus responses (exceptions included) */
    unsigned int timeouts;   /* number of times this command has timed out */
    unsigned int crcerrors;  /* number of checksum errors */
    unsigned int exceptions; /* number of modbus exceptions recieved from slave */
    u_int8_t lasterror;      /* last error on command */
    u_int16_t lastcrc;       /* used to determine if a conditional message should be sent */
    unsigned char firstrun;  /* Indicates that this command has been sent once */
    void *userdata;          /* Data that can be assigned by the user.  Use free function callback */
    void (*pre_send)(struct mb_cmd *cmd, void *userdata, u_int8_t *data, int size);
    void (*post_send)(struct mb_cmd *cmd, void *userdata, u_int8_t *data, int size);
    void (*send_fail)(struct mb_cmd *cmd, void *userdata);
    void (*userdata_free)(struct mb_cmd *cmd, void *userdata); /* Callback to free userdata */
    struct mb_cmd* next;
};

/* Port Functions - defined in modports.c */
int add_cmd(mb_port *p, mb_cmd *mc);


/* Command Functions - defined in modcmds.c */


/* Utility Functions - defined in modutil.c */
u_int16_t crc16(unsigned char *msg, unsigned short length);
int crc16check(u_int8_t *buff, int length);

#ifdef DEBUG
 #define DEBUGMSG(x) debug(x)
 #define DEBUGMSG2(x,y) debug(x,y)
void debug(char *message, ...);
#else
 #define DEBUGMSG(x)
 #define DEBUGMSG2(x,y)
#endif

#endif /* __MODLIB_H */
