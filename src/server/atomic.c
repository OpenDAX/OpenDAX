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
#include "retain.h"
#include "func.h"
#include <ctype.h>
#include <assert.h>

extern _dax_tag_db *_db;

/* Left shifts a buffer of BOOL data. bit must be less than 8 */
inline static void
_left_shift(dax_byte *dest, dax_byte *src, size_t size, uint8_t bit) {
    for(int n=size-1; n>0; n--) {        /* Start at the high order byte */
        dest[n] = (src[n-1] >> (8-bit)); /* move the high bits in the lower byte to the right*/
        dest[n] |= (src[n] << bit);      /* shift the higher byte and mask it on the dest */
    }
    dest[0] = (src[0] << bit);           /* Fix the LSB */
}


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
    return ERR_ARG; /* Should never get here */
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
    return ERR_ARG; /* Should never get here */
}


static int
_atomic_not(tag_handle h, void *data) {
    int n;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_BADTYPE;
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
    } else {
        for(n=0;n<h.size;n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] = ~*(dax_byte *)&_db[h.index].data[h.byte+n];
        }
    }
    return 0;
}


static int
_atomic_or(tag_handle h, void *data) {
    int n;
    dax_byte temp[h.size];
    dax_byte hmask, lmask;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_BADTYPE;
    } else if(h.type == DAX_BOOL && !(h.count % 8 == 0 && h.bit == 0)) {
        _left_shift(temp, data, h.size, h.bit);

        hmask = 0xFF << ((h.bit + (h.count%8))%8);
        lmask = 0xFF >> (8-h.bit);

        temp[h.size-1] &= ~hmask;
        temp[0]      &= ~lmask;

        for(n=0; n<h.size; n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] |= temp[n];
        }
    } else {
        for(n=0; n<h.size; n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] |= ((dax_byte *)data)[h.byte+n];
        }
    }
    return 0;
}


static int
_atomic_and(tag_handle h, void *data) {
    int n;
    dax_byte temp[h.size];
    dax_byte hmask, lmask;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_BADTYPE;
    } else if(h.type == DAX_BOOL && !(h.count % 8 == 0 && h.bit == 0)) {
        _left_shift(temp, data, h.size, h.bit);
        hmask = 0xFF << ((h.bit + (h.count%8))%8);
        lmask = 0xFF >> (8-h.bit);

        temp[h.size-1] |= hmask;
        temp[0]      |= lmask;

        for(n=0; n<h.size; n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] &= temp[n];
        }
    } else {
        for(n=0; n<h.size; n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] &= ((dax_byte *)data)[h.byte+n];
        }
    }
    return 0;
}


static int
_atomic_nor(tag_handle h, void *data) {
    int n;
    dax_byte temp[h.size];
    dax_byte hmask, lmask;
    size_t size;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_BADTYPE;
    } else if(h.type == DAX_BOOL && !(h.count % 8 == 0 && h.bit == 0)) {
        size = h.size;
        _left_shift(temp, data, size, h.bit);
        hmask = 0xFF << ((h.bit + (h.count%8))%8);
        lmask = 0xFF >> (8-h.bit);

        temp[size-1] &= ~hmask;
        temp[0]      &= ~lmask;

        /* To do the NOR with our mask we first apply the mask just like
           with the OR.  Then XOR the middle bytes with 0xFF to toggle them
           which makes it a NOR and then we do the same thing to the first
           and last bytes except with the compliment of the mask.  This
           keeps the bits that are outside of our range and then toggles
           the ones that are inside the range. */
        for(n=0;n< size;n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] |= temp[n];
        }
        for(n=1;n<size-1;n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] ^= 0xFF;
        }
        ((dax_byte *)_db[h.index].data)[size-1] ^= ~hmask;
        ((dax_byte *)_db[h.index].data)[0]      ^= ~lmask;

    } else {
        for(n=0;n<h.size;n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] = ~(*(dax_byte *)&_db[h.index].data[h.byte+n] | ((dax_byte *)data)[h.byte+n]);
        }
    }
    return 0;
}


static int
_atomic_nand(tag_handle h, void *data) {
    int n;
    dax_byte temp[h.size];
    dax_byte hmask, lmask;
    size_t size;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_BADTYPE;
    } else if(h.type == DAX_BOOL && !(h.count % 8 == 0 && h.bit == 0)) {
        size = h.size;
        _left_shift(temp, data, size, h.bit);
        hmask = 0xFF << ((h.bit + (h.count%8))%8);
        lmask = 0xFF >> (8-h.bit);

        temp[size-1] |= hmask;
        temp[0]      |= lmask;

        for(n=0;n< size;n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] &= temp[n];
        }
        for(n=1;n<size-1;n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] ^= 0xFF;
        }
        ((dax_byte *)_db[h.index].data)[size-1] ^= ~hmask;
        ((dax_byte *)_db[h.index].data)[0]      ^= ~lmask;
    } else {
        for(n=0;n<h.size;n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] = ~(*(dax_byte *)&_db[h.index].data[h.byte+n] & ((dax_byte *)data)[h.byte+n]);
        }
    }
    return 0;
}


static int
_atomic_xor(tag_handle h, void *data) {
    int n;
    dax_byte temp[h.size];
    dax_byte hmask, lmask;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_BADTYPE;
    } else if(h.type == DAX_BOOL && !(h.count % 8 == 0 && h.bit == 0)) {
        _left_shift(temp, data, h.size, h.bit);

        hmask = 0xFF << ((h.bit + (h.count%8))%8);
        lmask = 0xFF >> (8-h.bit);

        temp[h.size-1] &= ~hmask;
        temp[0]      &= ~lmask;

        for(n=0; n<h.size; n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] ^= temp[n];
        }
    } else {
        for(n=0; n<h.size; n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] ^= ((dax_byte *)data)[h.byte+n];
        }
    }
    return 0;
}

static int
_atomic_xnor(tag_handle h, void *data) {
    int n;
    dax_byte temp[h.size];
    dax_byte hmask, lmask;
    size_t size;

    if(h.type == DAX_REAL || h.type == DAX_LREAL) {
        return ERR_BADTYPE;
    } else if(h.type == DAX_BOOL && !(h.count % 8 == 0 && h.bit == 0)) {
        size = h.size;
        _left_shift(temp, data, size, h.bit);
        hmask = 0xFF << ((h.bit + (h.count%8))%8);
        lmask = 0xFF >> (8-h.bit);

        temp[size-1] &= ~hmask;
        temp[0]      &= ~lmask;

        for(n=0;n< size;n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] ^= temp[n];
        }
        for(n=1;n<size-1;n++) {
            ((dax_byte *)_db[h.index].data)[h.byte+n] ^= 0xFF;
        }
        ((dax_byte *)_db[h.index].data)[size-1] ^= ~hmask;
        ((dax_byte *)_db[h.index].data)[0]      ^= ~lmask;

    } else {
        for(n=0;n<h.size;n++) {
            *(dax_byte *)&_db[h.index].data[h.byte+n] = ~(*(dax_byte *)&_db[h.index].data[h.byte+n] ^ ((dax_byte *)data)[h.byte+n]);
        }
    }
    return 0;
}


int
atomic_op(tag_handle h, void *data, uint16_t op) {
    int result;
    /* We don't do these on custom data types */
    if(IS_CUSTOM(h.type)) {
        return ERR_BADTYPE;
    }
    /* Make sure we are a valid tag index*/
    if(h.index < 0 || h.index >= get_tagindex()) {
        return ERR_ARG;
    }
    /* Bounds Check Size - we can use the TYPESIZE macro here because we have
       already excluded CDTs */
    if((h.byte + h.size) > (_db[h.index].count * TYPESIZE(_db[h.index].type))) {
        return ERR_2BIG;
    }
    /* Since we directly manipulate the _db here we can't deal with virtual tags or
       special tags.*/
    if(_db[h.index].attr & (TAG_ATTR_VIRTUAL | TAG_ATTR_SPECIAL)) {
        return ERR_READONLY;
    }
    /* If the index is within range but the data area is a NULL pointer then the
       tag has been deleted */
    if(_db[h.index].data == NULL) {
        return ERR_DELETED;
    }
    switch(op) {
        case ATOMIC_OP_INC:
            if(h.type == DAX_BOOL) return ERR_BADTYPE;
            result = _atomic_inc(h, data);
            break;
        case ATOMIC_OP_DEC:
            if(h.type == DAX_BOOL) return ERR_BADTYPE;
            result = _atomic_dec(h, data);
            break;
        case ATOMIC_OP_NOT:
            result = _atomic_not(h, data);
            break;
        case ATOMIC_OP_OR:
            result = _atomic_or(h, data);
            break;
        case ATOMIC_OP_AND:
            result = _atomic_and(h, data);
            break;
        case ATOMIC_OP_NOR:
            result = _atomic_nor(h, data);
            break;
        case ATOMIC_OP_NAND:
            result = _atomic_nand(h, data);
            break;
        case ATOMIC_OP_XOR:
            result = _atomic_xor(h, data);
            break;
        case ATOMIC_OP_XNOR:
            result = _atomic_xnor(h, data);
            break;
        default:
            return ERR_NOTIMPLEMENTED;
    }
    if(result) return result;
    event_check(h.index, h.byte, h.size);
    if(_db[h.index].attr & TAG_ATTR_RETAIN) {
        ret_tag_write(h.index);
    }

    return 0;
}

