/* database.c - Modbus module for OpenDAX
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

#include <common.h>
#include <opendax.h>
#include <database.h>
#include <pthread.h>

//static u_int16_t *datatable;
static unsigned int tablesize;
static pthread_mutex_t datamutex;
static handle_t dt_handle;

/* Allocate and Initialize the datatable */
int dt_init(unsigned int size) {
    dt_handle = dax_tag_add("modbus",DAX_UINT, size);
    
    tablesize=size;
    
    pthread_mutex_init(&datamutex,NULL);
    return(0);
}

int dt_destroy(void) {
    pthread_mutex_destroy(&datamutex);
    return 0;
}

u_int16_t dt_getword(unsigned int address) {
    u_int16_t temp;
    pthread_mutex_lock(&datamutex);
    dax_tag_read_bytes(dt_handle + (address * 16),&temp,2);
    pthread_mutex_unlock(&datamutex);
    return(temp);
}

int dt_setword(unsigned int address, u_int16_t buff) {
    if(address < tablesize) {
        pthread_mutex_lock(&datamutex);
        //datatable[address]=buff;
        dax_tag_write_bytes(dt_handle + (address * 16),&buff,2);
        pthread_mutex_unlock(&datamutex);
        return 0;
    }
    return -1;
}

/* TODO: Need to add multiple word versions of the above two functions. */

char dt_getbit(unsigned int address) {
    pthread_mutex_lock(&datamutex);
    if(dax_tag_read_bit(dt_handle + address)) { /* If bit is set */
        pthread_mutex_unlock(&datamutex);
        return 1;
    } else {
        pthread_mutex_unlock(&datamutex);
        return 0;
    }
    return -1; /* we can't get here */
}

/* This function sets bits in buff.  Address and length are both given in
   number of bits. */
int dt_setbits(unsigned int address, u_int8_t *buff, u_int16_t length) {
    int result;
    pthread_mutex_lock(&datamutex);
    
    result = dax_tag_write_bits(dt_handle + address,buff,length);
    
    pthread_mutex_unlock(&datamutex);
    return result;
}
