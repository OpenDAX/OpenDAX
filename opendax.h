/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 1997 Phil Birkelbach
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
 */


#ifndef __OPENDAX_H
#define __OPENDAX_H

/* For now we'll use this as the key for all the SysV IPC stuff */
#define DAX_IPC_KEY 0x707070

/* Data type definitions */
#define DAX_BOOL    0x0010
/* 8 Bits */
#define DAX_BYTE    0x0003
#define DAX_SINT    0x0013
/* 16 Bits */
#define DAX_WORD    0x0004
#define DAX_INT     0x0014
#define DAX_UINT    0x0024
/* 32 Bits */
#define DAX_DWORD   0x0005
#define DAX_DINT    0x0015
#define DAX_UDINT   0x0025
#define DAX_TIME    0x0035
#define DAX_REAL    0x0045
/* 64 Bits */
#define DAX_LWORD   0x0006
#define DAX_LINT    0x0016
#define DAX_ULINT   0x0026
#define DAX_LREAL   0x0036

/* The highest order bit shows that it's a custom datatype */
#define DAX_CUSTOM  0x80000000

/* The size of the base datatypes are based on the lower 4 bits.  A
   simple bit shift will give the size of the datatype */
#define DAX_1BIT    0x0000
#define DAX_8BITS   0x0003
#define DAX_16BITS  0x0004
#define DAX_32BITS  0x0005
#define DAX_64BITS  0x0006

/* Library function errors */
#define ERR_NO_QUEUE -1 /* The Message Queue does not exist */
#define ERR_2BIG     -2 /* The argument is too big */
#define ERR_ARG      -3 /* Some argument is missing */

/* Macro to get the size of the datatype */
#define TYPESIZE(TYPE) (0x0001 << (TYPE & 0x0F))

typedef int handle_t;

/* Only registered modules will get responses from the core */
int dax_mod_register(char *);   /* Registers the Module with the core */
int dax_mod_unregister(void);   /* Unregister the Module with the core */
handle_t dax_tag_add(char *name,unsigned int type, unsigned int count);

#endif /* !__OPENDAX_H */
