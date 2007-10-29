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

 * This file contains some of the database handling code for the library
 */

#include <libdax.h>
#include <dax/tagbase.h>
#include <dax/message.h>


/* Use this tag type when we don't care about the name */
typedef struct {
    handle_t handle;
    unsigned int type;
    unsigned int count;
} io_tag;

/* returns zero if the bit is clear, 1 if it's set */
char dax_tag_read_bit(handle_t handle) {
    u_int8_t data;
    /* read the one byte that contains the handle the bit we want */
    dax_tag_read(handle & ~0x07, &data, 1);
    if(data & (1 << (handle % 8))) {
        return 1;
    }
    return 0;
}

/* sets the bit if data evaluates true and clears it otherwise */
int dax_tag_write_bit(handle_t handle, u_int8_t data) {
    u_int8_t input, mask;
    if(data) {
        input = 0xFF;
    } else {
        input = 0x00;
    }
    mask = 1 << (handle % 8);
    dax_tag_mask_write(handle & ~0x07, &input, &mask, 1);
    return 0;
}


/* Reads bits starting at handle, size is in bits not bytes.  This function
   will be more efficient if the size and handle are evenly divisible by 8 */
int dax_tag_read_bits(handle_t handle, void *data, size_t size) {
    void *buffer;
    size_t n, buff_len, buffer_idx, data_idx, data_bit, buffer_bit;
    /* Screw it I'm gonna allocate the whole thing and see what happens.
       I'm also gonna use alloca() because it should be faster on the stack
       none of this may matter and I'd be easy enough to change */
    buff_len = size / 8; /* Start here */
    
 /* If handle and size are even bytes then this is just a byte read */
    if( handle % 8 == 0 && size % 8 == 0) {
        return dax_tag_read(handle, data, buff_len);
    }

    buff_len = (((handle + size - 1) & ~0x07) - (handle & ~0x07)) / 8 + 1;
    buffer = alloca(buff_len);
    if(buffer == NULL) {
        return -1;
    }
    if(dax_tag_read(handle, buffer, buff_len)) {
        return -1;
    }
    buffer_bit = handle % 8;
    buffer_idx = 0;
    data_bit = 0;
    data_idx = 0;
 /* Loop through the bits moving from buffer to data */
    for(n = 0; n< size; n++) {
        if(((u_int8_t *)buffer)[buffer_idx] & (1 << buffer_bit)) { /* If *buff bit is set */
            ((u_int8_t *)data)[data_idx] |= (u_int8_t)(1 << data_bit);
        } else {  /* If the bit in the buffer is not set */
            ((u_int8_t *)data)[data_idx] &= ~(u_int8_t)(1 << data_bit);
        }
        data_bit++;
        if(data_bit == 8) {
            data_bit = 0;
            data_idx++;
        }
        buffer_bit++;
        if(buffer_bit == 8) {
            buffer_bit = 0;
            buffer_idx++;
        }
    }
    return 0;
}

/* This is how many bytes we'll send in each call to the masked write. */
#define DAX_BIT_MSG_SIZE (MSG_TAG_DATA_SIZE / 2)

/* Writes bits starting at handle, size is in bits not bytes */
/* This function could be a lot simpler if we'd just allocate a buffer
   that is big enough for all the bits to fit and then let the 
   dax_tag_mask_write() function handle splitting them up.  I'm betting
   on this being quite a bit faster.  It might not matter. */
/* TODO: Need to test whether this thing will handle more than
   one message worth of bits.  Modbus just can't produce that many
   bits in one message */
int dax_tag_write_bits(handle_t handle, void *data, size_t size) {
    u_int8_t buffer[DAX_BIT_MSG_SIZE];
    u_int8_t mask[DAX_BIT_MSG_SIZE];
    size_t buffer_idx, buffer_bit, data_idx, data_bit;
    handle_t next_handle;
    int n;
    
    buffer_idx = 0;
    buffer_bit = handle % 8;
    next_handle = handle & ~0x07;
    data_idx = 0;
    data_bit = 0;
    
    for(n=0; n < DAX_BIT_MSG_SIZE; n++) mask[n]=0x00;
    
    for(n=0; n<size; n++) {
        if(((u_int8_t *)data)[data_idx] & (1 << data_bit)) { /* If *buff bit is set */
            buffer[buffer_idx] |= (u_int16_t)(1 << buffer_bit);
        } else {  /* If the bit in the buffer is not set */
            buffer[buffer_idx] &= ~(u_int16_t)(1 << buffer_bit);
        }
        /* This sets the mask bit */
        mask[buffer_idx] |= (u_int16_t)(1 << buffer_bit);
    
        data_bit++;
        if(data_bit == 8) {
            data_bit = 0;
            data_idx++;
        }
        
        buffer_bit++;
        if(buffer_bit == 8) {
            buffer_bit = 0;
            buffer_idx++;
            if(buffer_idx == DAX_BIT_MSG_SIZE) {
                /* Out of room in the buffer, send the message */
                dax_tag_mask_write(next_handle, buffer, mask, buffer_idx);
                next_handle += DAX_BIT_MSG_SIZE;
                buffer_idx = 0;
                for(n=0; n < DAX_BIT_MSG_SIZE; n++) mask[n]=0x00;
            }
        }
    }
    dax_tag_mask_write(next_handle, buffer, mask, buffer_idx + 1);
    /* TODO: Need to do some real error checking here and return a good number */
    return size;
}



