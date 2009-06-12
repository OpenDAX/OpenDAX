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

#include <sys/types.h>

/* Port Types */
#define MB_MASTER 0
#define MB_SLAVE  1
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

/* Modbus Errors */
#define ME_EXCEPTION      0x80;
#define ME_WRONG_DEVICE   1;
#define ME_WRONG_FUNCTION 2;
#define ME_CHECKSUM       4;
#define ME_TIMEOUT        8;

/* Command Methods */
#define	MB_DISABLE     0
#define MB_CONTINUOUS  1
#define	MB_ONCHANGE    2
#define MB_TRIGGER     3

typedef struct mb_port mb_port;
typedef struct mb_cmd  mb_cmd;

/* New Interface */

mb_port *mb_new_port(const char *name);
void mb_destroy_port(mb_port *port);
int mb_set_serial_port(mb_port *port, const char *device, int baudrate, short databits, short parity, short stopbits);
int mb_set_network_port(mb_port *port, const char *ipaddress, unsigned int bindport, unsigned char socket);
int mb_set_protocol(mb_port *port, unsigned char type, unsigned char protocol, u_int8_t slaveid);
void mb_set_output_callback(mb_port *, void (*outfunc)(mb_port *,u_int8_t *,unsigned int));
void mb_set_input_callback(mb_port *, void (*infunc)(mb_port *,u_int8_t *,unsigned int));

mb_cmd  *mb_new_cmd(mb_port *port);
void mb_disable_cmd(mb_cmd *cmd);
void mb_enable_cmd(mb_cmd *cmd);

/* End New Interface */
//--int mb_add_cmd(mb_port *,mb_cmd *);
mb_cmd *mb_get_cmd(mb_port *, unsigned int);
int mb_open_port(mb_port *);
int mb_start_port(mb_port *);
int mb_send_command(mb_port *, mb_cmd *);

#endif
