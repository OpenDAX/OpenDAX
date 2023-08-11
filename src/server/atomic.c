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
    return 1;
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
    return 1;
}

static int
_atomic_not(tag_handle h, void *data) {
    int n;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_ILLEGAL;
    } else if(h.type == DAX_BOOL) {
        // There has got to be a more efficient way to do this but we're gonna do it the
        // easy way for now.  Just looping through each bit in turn.
        int byte = h.byte;
        int bit = h.bit;
        for(n=0;n<h.count;n++) {
            _db[h.index].data[byte] ^= (0x01 << bit);
            bit++;
            if(bit==8) {
                byte++;
                bit = 0;
            }
        }
        return 0;
    } else {
        for(n=0;n<h.size;n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] = ~*(dax_byte *)&_db[h.index].data[h.byte+n];
        }
        return 0;
    }
    return 1;
}

static int
_atomic_or(tag_handle h, void *data) {
    int n;
    dax_byte *_data;
    _data = (dax_byte *)data;
    dax_byte temp, mask ;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_ILLEGAL;
    } else if(h.type == DAX_BOOL && !(h.count % 8 == 0 && h.bit == 0)) {
        DF("h.count = %d", h.count);
        DF("h.size = %d", h.size);
        DF("h.bit = %d", h.bit);
        DF("h.byte = %d", h.byte);

        /* This is a bit mask to clean up the higher order (spare) bits that
         * may be in data */
        mask = ~(0xFF << (h.count%8));
        DF("_data[1] = %X", _data[1]);
        _data[h.count/8] &= mask;
        DF("mask = %X", mask);
        DF("_data[1] = %X", _data[1]);
        for(n=0; n<h.size; n++) {
            DF("data[%d] = %d", n, _data[n]);
            temp = 0;
            if(n != 0) {
                temp = (_data[n-1] >> (8-h.bit));
                DF("temp = %X", temp);
            }
            temp |= (_data[n] << h.bit);
            DF("temp = %X", temp);
            ((dax_byte *)_db[h.index].data)[h.byte+n] |= temp;
        }
        return 0;
    } else {
        DF("Easy way!!!!");
        for(n=0;n<h.size;n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] |= ((dax_byte *)data)[h.byte+n];
        }
        return 0;
    }
    return 1;
}



int
atomic_op(tag_handle h, void *data, uint16_t op) {

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
        case ATOMIC_OP_NOT:
            return _atomic_not(h, data);
        case ATOMIC_OP_OR:
            return _atomic_or(h, data);
        case ATOMIC_OP_AND:
            return ERR_NOTIMPLEMENTED;
        case ATOMIC_OP_NOR:
            return ERR_NOTIMPLEMENTED;
        case ATOMIC_OP_NAND:
            return ERR_NOTIMPLEMENTED;
        case ATOMIC_OP_XOR:
            return ERR_NOTIMPLEMENTED;

    }
    /* The rest of the data types are not done yet */
    /* TODO Finish the bitwise operators */
    return ERR_NOTIMPLEMENTED;
    //return 0;
}

