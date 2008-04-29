/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *

 * This file contains the function definitions that are used internally
 * by the libarary.  Public functions should go in opendax.h
 */

#ifndef __LIBDAX_H
#define __LIBDAX_H  

#include <common.h>
#include <opendax.h>

#ifndef TAG_CACHE_LIMIT
#  define TAG_CACHE_LIMIT 32
#endif

/* Data Conversion Functions */
#define REF_INT_SWAP 0x0001
#define REF_FLT_SWAP 0x0002

/* 16 Bit conversion functions */
#define mtos_word mtos_uint
#define stom_word stom_uint

int16_t mtos_int(int16_t);
u_int16_t mtos_uint(u_int16_t);

int16_t stom_int(int16_t);
u_int16_t stom_uint(u_int16_t);

/* 32 Bit conversion functions */
#define mtos_word mtos_uint
#define stom_word stom_uint
#define mtos_time mtos_uint
#define stom_time stom_uint

int32_t mtos_dint(int32_t);
u_int32_t mtos_udint(u_int32_t);
float mtos_real(float);

int32_t stom_dint(int32_t);
u_int32_t stom_udint(u_int32_t);
float stom_real(float);

//--int _message_send(long int module,int command,void *payload, size_t size);
//--int _message_recv(int command, void *payload, size_t *size);

char *opt_get_socketname(void);

#endif /* !__LIBDAX_H */
