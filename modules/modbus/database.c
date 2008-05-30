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
#include <database.h>
#include <pthread.h>

//static u_int16_t *datatable;
static unsigned int tablesize;
static pthread_mutex_t datamutex;
static handle_t dt_handle;

/* Allocate and Initialize the datatable */
int
dt_init(unsigned int size, char *tagname)
{
    dt_handle = dax_tag_add(tagname, DAX_UINT, size);
    if(dt_handle < 0) {
        return dt_handle;
    }
    
    tablesize = size;
    
    pthread_mutex_init(&datamutex, NULL);
    return(0);
}

int
dt_destroy(void)
{
    pthread_mutex_destroy(&datamutex);
    return 0;
}

int
dt_add_tag(char *name, int index, int function, int length)
{
    int result;
    
    switch(function) {
        case 1:
        case 2:
        case 15:
            result = dax_tag_add(name, DAX_BOOL, index + length);
            return result;
        case 5:
            result = dax_tag_add(name, DAX_BOOL, index + 1);
            return result;
        case 3:
        case 4:
        case 16:
            result = dax_tag_add(name, DAX_UINT, index + length);
            return result;
        case 6:
            result = dax_tag_add(name, DAX_UINT, index + 1);
            return result;
        default:
            return -1;
    }
}


int
dt_getwords(handle_t handle, int index, void *data, int length)
{
    int result;
    if(handle) {
        pthread_mutex_lock(&datamutex);
        result = dax_read_tag(handle, index, data, length, DAX_UINT);
        pthread_mutex_unlock(&datamutex);
    } else {
        result = ERR_ARG;
    }
    return result;
}

int
dt_setwords(handle_t handle, int index, void *data, int length)
{
    int result;
    if(handle) {
        pthread_mutex_lock(&datamutex);
        result = dax_write_tag(handle, index, data, length, DAX_UINT);
        pthread_mutex_unlock(&datamutex);
    } else {
        result = ERR_ARG;
    }
    return result;
}


int
dt_getbits(handle_t handle, int index, void *data, int length)
{
    int result;
    if(handle) {
        pthread_mutex_lock(&datamutex);
        result = dax_read_tag(handle, index, data, length, DAX_BOOL);
        pthread_mutex_unlock(&datamutex);
    } else {
        result = ERR_ARG;
    }
    return result;
}

int
dt_setbits(handle_t handle, int index, void *data, int length)
{
    int result;
    if(handle) {
        pthread_mutex_lock(&datamutex);
        result = dax_write_tag(handle, index, data, length, DAX_BOOL);
        pthread_mutex_unlock(&datamutex);
    } else {
        result = ERR_ARG;
    }
    return result;
}
