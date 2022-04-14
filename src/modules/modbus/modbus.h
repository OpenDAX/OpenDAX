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
 *
 * This is the public header file for the modbus library functions
 */

#ifndef __MODBUS_H
#define __MODBUS_H

#include <config.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <string.h>
#include <strings.h>
#ifdef HAVE_SYS_SELECT_H
 #include <sys/select.h>
 #ifndef FD_COPY
  #define FD_COPY(f, t) (void)(*(t) = *(f))
 #endif
#endif

#ifndef __USE_XOPEN
 #define __USE_XOPEN
#endif
#define _XOPEN_SOURCE
#include <unistd.h>
#undef _XOPEN_SOURCE

#include <termios.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <common.h>
#include <opendax.h>

/* Port Types */
#define MB_MASTER 0
#define MB_SLAVE  1
#define MB_CLIENT 0
#define MB_SERVER 1

/* Protocols */
#define MB_RTU   1
#define MB_ASCII 2
#define MB_TCP   4
/* Socket type for network ports */
#define UDP_SOCK 1
#define TCP_SOCK 2

/* Parity */
#define MB_NONE 0
#define MB_EVEN 1
#define MB_ODD  2

/* Library Errors */
#define MB_ERR_GENERIC    -1
#define MB_ERR_ALLOC      -2      /* Unable to Allocate Storage */
#define MB_ERR_BAUDRATE   -3      /* Bad Baudrate */
#define MB_ERR_DATABITS   -4      /* Wrong number of databits */
#define MB_ERR_STOPBITS   -5      /* Wrong number of stopbits */
#define MB_ERR_SOCKET     -6      /* Bad socket identifier */
#define MB_ERR_PORTTYPE   -7      /* Bad port type - Master/Slave */
#define MB_ERR_PROTOCOL   -8      /* Bad protocol - RTU/ASCII/TCP */
#define MB_ERR_FUNCTION   -9      /* Bad Function Code Given */
#define MB_ERR_OPEN       -10     /* Failure to Open Port */
#define MB_ERR_PORTFAIL   -11     /* Major Port Failure */
#define MB_ERR_RECV_FAIL  -12     /* Recieve Failure */
#define MB_ERR_NO_SOCKET  -13     /* The socket does not exist */
#define MB_ERR_BAD_ARG    -14     /* Bad Argument */
#define MB_ERR_STOPPED    -15     /* Port was stopped externally */
#define MB_ERR_OVERFLOW   -16     /* Buffer Overflow */

/* Modbus Errors */
#define ME_EXCEPTION      0x80
#define ME_WRONG_FUNCTION 1
#define ME_BAD_ADDRESS    2
#define ME_WRONG_DEVICE   3
#define ME_CHECKSUM       4
#define ME_TIMEOUT        8

/* Command Methods */
#define MB_CONTINUOUS  0x01   /* Command is sent periodically */
#define	MB_ONCHANGE    0x02   /* Command is sent when the data tag changes */
#define MB_ONWRITE     0x04   /* Command is sent when written */
#define MB_TRIGGER     0x08   /* Command is sent when trigger tag is set */

/* Port Attribute Flags */
#define MB_FLAGS_STOP_LOOP    0x01

/* Register Identifications */
#define MB_REG_HOLDING 1
#define MB_REG_INPUT   2
#define MB_REG_COIL    3
#define MB_REG_DISC    4

#ifdef __BIG_ENDIAN__ /* PPC Endianness */
# define __MB_BIG_ENDIAN
#endif

#ifdef __BYTE_ORDER /* x86 Endianness */
# if __BYTE_ORDER == __BIG_ENDIAN
#   define __MB_BIG_ENDIAN
# endif
#endif

/* Used to insert a 16bit value into the modbus buffer
 * another endian machine would simply use memcpy() */
#ifndef __MB_BIG_ENDIAN
# define COPYWORD(OUT,IN) swab((const void *)(IN),(void *)(OUT),(ssize_t)2)
#else
# define COPYWORD(OUT,IN) memcpy((OUT),(IN),2);
#endif


#define MB_FRAME_LEN 1024

#ifndef MAX_RETRIES
#  define MAX_RETRIES 100
#endif

/* Device Port Types */
#define MB_SERIAL  0
#define MB_NETWORK 1

/* Maximum size of the receive buffer */
#define MB_BUFF_SIZE 256
/* Starting number of connections in the pool */
#define MB_INIT_CONNECTION_SIZE 16
/* Maximum number of connections that can be in the pool */
#define MB_MAX_CONNECTION_SIZE 2048

/* This is used in the port for client connections for the TCP Server */
struct client_buffer {
    int fd;                /* File descriptor of the socket */
    int buffindex;         /* index where the next character will be placed */
    unsigned char buff[MB_BUFF_SIZE];   /* data buffer */
    struct client_buffer *next;
};

/* This structure represents a single connection to a TCP server.
 * There is a dynamic array of these in the port that are basically
 * used as a connection pool. If fd is not zero then we are connected.
 */
typedef struct tcp_connection {
    struct in_addr addr;
    uint16_t port;
    int fd;
} tcp_connection;

/* Internal struct that defines a single Modbus(tm) Port */
typedef struct mb_port {
    char *name;               /* Port name if needed : Maybe we don't need this */
    unsigned int flags;       /* Port Attribute Flags */
    char *device;             /* device filename of the serial port */
    unsigned char enable;     /* 0=Pause, 1=Run */
    unsigned char type;       /* 0=Master, 1=Slave */
    unsigned char devtype;    /* 0=serial, 1=network */
    unsigned char protocol;   /* [Only RTU is implemented so far] */
    uint8_t slaveid;          /* Slave ID 1-247 (Slave Only) */
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

    char *hold_name;
    unsigned int hold_size;    /* size of the internal holding register bank */
    tag_handle hold_tag;
    char *input_name;
    unsigned int input_size;   /* size of the internal input register bank */
    tag_handle input_tag;
    char *coil_name;
    unsigned int coil_size;    /* size of the internal bank of coils in 16-bit registers */
    tag_handle coil_tag;
    char *disc_name;
    unsigned int disc_size;    /* size of the internal bank of coils */
    tag_handle disc_tag;

    fd_set fdset;
    int maxfd;
    struct client_buffer *buff_head; /* Head of a linked list of client connection buffers */

    struct mb_cmd *commands;  /* Linked list of Modbus commands */
    int fd;                   /* File descriptor to the port */
    int ctrl_flags;
    int dienow;
    unsigned int attempt;        /* Attempt counter */
    unsigned char running;       /* Flag to indicate the port is running */
    unsigned char inhibit;       /* When set the port will not be started */
    unsigned int inhibit_time;   /* Number of seconds before the port will be retried */
    unsigned int inhibit_temp;

    tcp_connection *connections;
    int connection_size;
    int connection_count;
    uint8_t persist;

    /* These are callback function pointers for the port message data */
    void (*out_callback)(struct mb_port *port, uint8_t *buff, unsigned int);
    void (*in_callback)(struct mb_port *port, uint8_t *buff, unsigned int);
    void (*slave_read)(struct mb_port *port, int reg, int index, int size, uint16_t *data);
    void (*slave_write)(struct mb_port *port, int reg, int index, int size, uint16_t *data);
    void (*userdata_free)(struct mb_port *port, void *userdata);
} mb_port;

typedef struct mb_cmd {
    unsigned char enable;    /* 0=disable, 1=enable */
    unsigned char mode;      /* MB_CONTINUOUS, MB_ONCHANGE, MB_ONWRITE, MB_TRIGGER */
    struct in_addr ip_address;     /* IP address for TCP requests */
    uint16_t port;           /* TCP port to connect to */
    uint8_t node;            /* Modbus device ID */
    uint8_t function;        /* Function Code */
    uint16_t m_register;     /* Modbus Register */
    uint16_t length;         /* length of modbus data */

    unsigned int interval;   /* number of port scans between messages */
    uint8_t *data;           /* pointer to the actual modbus data that this command refers */
    int datasize;            /* size of the *data memory area */
    unsigned int icount;     /* number of intervals passed */
    unsigned int requests;   /* total number of times this command has been sent */
    unsigned int responses;  /* number of valid modbus responses (exceptions included) */
    unsigned int timeouts;   /* number of times this command has timed out */
    unsigned int crcerrors;  /* number of checksum errors */
    unsigned int exceptions; /* number of modbus exceptions recieved from slave */
    uint8_t lasterror;       /* last error on command */
    uint16_t lastcrc;        /* used to determine if a conditional message should be sent */
    unsigned char firstrun;  /* Indicates that this command has been sent once */

    char *trigger_tag;       /* Tagname for tag that will be used to trigger this command must be BOOL */
    char *data_tag;          /* Tagname for the tag that will represent the data for this command. */
    uint32_t tagcount;       /* Number of tag items to read/write */
    tag_handle trigger_h;    /* Handle to trigger tag */
    tag_handle data_h;       /* Handle to data tag */

    struct mb_cmd* next;
} mb_cmd;

/* Create a New Modbus Port with the given name */
mb_port *mb_new_port(const char *name, unsigned int flags);
/* Frees the memory allocated with mb_new_port() */
void mb_destroy_port(mb_port *port);
/* Port Configuration Functions */
int mb_set_serial_port(mb_port *port, const char *device, int baudrate, short databits, short parity, short stopbits);
int mb_set_network_port(mb_port *port, const char *ipaddress, unsigned int bindport, unsigned char socket);
int mb_set_protocol(mb_port *port, unsigned char type, unsigned char protocol, uint8_t slaveid);
int mb_set_scan_rate(mb_port *port, int rate);
int mb_set_retries(mb_port *port, int retries);
int mb_set_maxfailures(mb_port *port, int maxfailures, int inhibit);

const char *mb_get_port_name(mb_port *port);
unsigned char mb_get_port_type(mb_port *port);
unsigned char mb_get_port_protocol(mb_port *port);
uint8_t mb_get_port_slaveid(mb_port *port);

int mb_set_holdreg_size(mb_port *port, unsigned int size);
int mb_set_inputreg_size(mb_port *port, unsigned int size);
int mb_set_coil_size(mb_port *port, unsigned int size);
int mb_set_discrete_size(mb_port *port, unsigned int size);

unsigned int mb_get_holdreg_size(mb_port *port);
unsigned int mb_get_inputreg_size(mb_port *port);
unsigned int mb_get_coil_size(mb_port *port);
unsigned int mb_get_discrete_size(mb_port *port);

int mb_open_port(mb_port *port);
int mb_close_port(mb_port *port);
int mb_get_connection(mb_port *mp, struct in_addr address, uint16_t port);
/* Set callback functions that are called any time data is read or written over the port */
void mb_set_msgout_callback(mb_port *, void (*outfunc)(mb_port *,uint8_t *,unsigned int));
void mb_set_msgin_callback(mb_port *, void (*infunc)(mb_port *,uint8_t *,unsigned int));

/* Slave Port callbacks */
/* Sets the callback that is called when the Slave/Server receives a request to read data from the slave*/
void mb_set_slave_read_callback(mb_port *mp, void (*infunc)(struct mb_port *port, int reg, int index, int count, uint16_t *data));
/* Sets the callback that is called when the Slave/Server receives a request to write data to the slave*/
void mb_set_slave_write_callback(mb_port *mp, void (*infunc)(struct mb_port *port, int reg, int index, int count, uint16_t *data));

void *mb_get_port_userdata(mb_port *mp);

/* Create a new command and add it to the port */
mb_cmd *mb_new_cmd(mb_port *port);
/* Free the memory allocated with mb_new_cmd() */
void mb_destroy_cmd(mb_cmd *cmd);
int mb_set_command(mb_cmd *cmd, uint8_t node, uint8_t function, uint16_t reg, uint16_t length);
int mb_set_interval(mb_cmd *cmd, int interval);
void mb_set_mode(mb_cmd *cmd, unsigned char mode);

int mb_is_write_cmd(mb_cmd *cmd);
int mb_is_read_cmd(mb_cmd *cmd);


int mb_scan_port(mb_port *mp);

/* Functions to add...
add free function to cmd
assign memory to the cmd's userdata
return pointer to cmd's userdata
*/

/* End New Interface */
int mb_run_port(mb_port *);
int mb_send_command(mb_port *, mb_cmd *);

void mb_print_portconfig(FILE *fd, mb_port *mp);

/* Port Functions - defined in modports.c */
int add_cmd(mb_port *p, mb_cmd *mc);

/* TCP Server Functions - defined in mbserver.c */
int server_loop(mb_port *port);

/* Serial Slave loop function */
int slave_loop(mb_port *port);

/* Protocol Functions - defined in modbus.c */
int create_response(mb_port * port, unsigned char *buff, int size);

/* Utility Functions - defined in modutil.c */
uint16_t crc16(unsigned char *msg, unsigned short length);
int crc16check(uint8_t *buff, int length);

#endif
