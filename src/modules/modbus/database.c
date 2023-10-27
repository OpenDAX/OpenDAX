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

#include <database.h>
#include <pthread.h>

extern dax_state *ds;


void
slave_write_database(tag_index idx, int reg, int offset, int count, uint16_t *data)
{
    int result;
    tag_handle h;

    /* We're going to cheat and build our own tag_handle */
    /* We're assuming that the server loop won't call this function
     * with bad data. */
    switch(reg) {
        case MB_REG_HOLDING: /* These are the 16 bit registers */
            h.index = idx;
            h.byte = offset * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_INPUT:
            h.index = idx;
            h.byte = offset * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_COIL:
            h.index = idx;
            h.byte = offset / 8;
            h.bit = offset % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
        case MB_REG_DISC:
            h.index = idx;
            h.byte = offset / 8;
            h.bit = offset % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
    }

    result = dax_write_tag(ds, h, data);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to write tag data to server\n");
    }
}


void
slave_read_database(tag_index idx, int reg, int offset, int count, uint16_t *data) {
    int result;
    tag_handle h;

    /* We're going to cheat and build our own tag_handle */
    /* We're assuming that the server loop won't call this function
     * with bad data. */
    switch(reg) {
        case MB_REG_HOLDING: /* These are the 16 bit registers */
            h.index = idx;
            h.byte = offset * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_INPUT:
            h.index = idx;
            h.byte = offset * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_COIL:
            h.index = idx;
            h.byte = offset / 8;
            h.bit = offset % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
        case MB_REG_DISC:
            h.index = idx;
            h.byte = offset / 8;
            h.bit = offset % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
    }
    result = dax_read_tag(ds, h, data);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to write tag data to server\n");
    }
}

