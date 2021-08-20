/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2021 Phil Birkelbach
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

 * This is the source file for the event notification handling routines
 */

#include <common.h>
#include "tagbase.h"
#include "func.h"
#include <ctype.h>
#include <assert.h>

extern _dax_tag_db *_db;

/* Data type definitions */
// #define DAX_BOOL    0x0010
/* 8 Bits */
// #define DAX_BYTE    0x0003
// #define DAX_SINT    0x0013
/* 16 Bits */
// #define DAX_WORD    0x0004
// #define DAX_INT     0x0014
// #define DAX_UINT    0x0024
/* 32 Bits */
// #define DAX_DWORD   0x0005
// #define DAX_DINT    0x0015
// #define DAX_UDINT   0x0025
// #define DAX_REAL    0x0035
/* 64 Bits */
// #define DAX_LWORD   0x0006
// #define DAX_LINT    0x0016
// #define DAX_ULINT   0x0026
// #define DAX_TIME    0x0036 /* milliseconds since the epoch */
// #define DAX_LREAL   0x0046


static int
_atomic_inc(tag_handle h, void *data) {
    /* We only operate on one member at a time */
    if(h.count != 1) return ERR_ILLEGAL;
    switch(h.type) {
        case DAX_BYTE:
            *(dax_byte *)&_db[h.index].data[h.byte] += *(dax_byte *)data;
            return 0;
        case DAX_SINT:
        case DAX_CHAR:
            *(dax_sint *)&_db[h.index].data[h.byte] += *(dax_sint *)data;
            return 0;
        case DAX_UINT:
        case DAX_WORD:
            *(dax_uint *)&_db[h.index].data[h.byte] += *(dax_uint *)data;
            return 0;
        case DAX_INT:
            *(dax_int *)&_db[h.index].data[h.byte] += *(dax_int *)data;
            return 0;
        case DAX_UDINT:
        case DAX_DWORD:
        case DAX_TIME:
            *(dax_udint *)&_db[h.index].data[h.byte] += *(dax_udint *)data;
            return 0;
        case DAX_DINT:
            *(dax_dint *)&_db[h.index].data[h.byte] += *(dax_dint *)data;
            return 0;
        case DAX_ULINT:
        case DAX_LWORD:
            *(dax_ulint *)&_db[h.index].data[h.byte] += *(dax_ulint *)data;
            return 0;
        case DAX_LINT:
            *(dax_lint *)&_db[h.index].data[h.byte] += *(dax_lint *)data;
            return 0;
        case DAX_REAL:
            *(dax_real *)&_db[h.index].data[h.byte] += *(dax_real *)data;
            return 0;
        case DAX_LREAL:
            *(dax_lreal *)&_db[h.index].data[h.byte] += *(dax_lreal *)data;
            return 0;
    }
    return 0;
}

static int
_atomic_dec(tag_handle h, void *data) {
    /* We only operate on one member at a time */
    if(h.count != 1) return ERR_ILLEGAL;
    switch(h.type) {
        case DAX_BYTE:
            *(dax_byte *)&_db[h.index].data[h.byte] -= *(dax_byte *)data;
            return 0;
        case DAX_SINT:
        case DAX_CHAR:
            *(dax_sint *)&_db[h.index].data[h.byte] -= *(dax_sint *)data;
            return 0;
        case DAX_UINT:
        case DAX_WORD:
            *(dax_uint *)&_db[h.index].data[h.byte] -= *(dax_uint *)data;
            return 0;
        case DAX_INT:
            *(dax_int *)&_db[h.index].data[h.byte] -= *(dax_int *)data;
            return 0;
        case DAX_UDINT:
        case DAX_DWORD:
        case DAX_TIME:
            *(dax_udint *)&_db[h.index].data[h.byte] -= *(dax_udint *)data;
            return 0;
        case DAX_DINT:
            *(dax_dint *)&_db[h.index].data[h.byte] -= *(dax_dint *)data;
            return 0;
        case DAX_ULINT:
        case DAX_LWORD:
            *(dax_ulint *)&_db[h.index].data[h.byte] -= *(dax_ulint *)data;
            return 0;
        case DAX_LINT:
            *(dax_lint *)&_db[h.index].data[h.byte] -= *(dax_lint *)data;
            return 0;
        case DAX_REAL:
            *(dax_real *)&_db[h.index].data[h.byte] -= *(dax_real *)data;
            return 0;
        case DAX_LREAL:
            *(dax_lreal *)&_db[h.index].data[h.byte] -= *(dax_lreal *)data;
            return 0;
    }
    return 0;
}


int
atomic_op(tag_handle h, void *data, u_int16_t op) {

    /* We don't do these on custom data types */
    if(IS_CUSTOM(h.type)) {
        return ERR_ILLEGAL;
    }
    switch(op) {
        case ATOMIC_OP_INC:
            if(h.type == DAX_BOOL) return ERR_ILLEGAL;
            return _atomic_inc(h, data);
        case ATOMIC_OP_DEC:
            if(h.type == DAX_BOOL) return ERR_ILLEGAL;
            return _atomic_dec(h, data);
    }
    /* The rest of the data types are not done yet */
    /* TODO Finish the bitwise operators */
    return ERR_NOTIMPLEMENTED;
    //return 0;
}

