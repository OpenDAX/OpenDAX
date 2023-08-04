/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *

 * This file contains some of the convenience functions for the module
 * All the functions in this file can be done in the modules with other
 * functions in this library, typically by manipulating tags in the tagserver.
 * These are just higher level functions to make life easier on the module
 * programmer.
 */

#include <libdax.h>
#include <common.h>
#include <stdarg.h>

#define _MY_TAGNAME "_my_tagname"

/*!
 * Finds the module tags .running bit and sets or clears it depending
 * on the boolean state of val.  If val is zero then we will clear
 * the running bit, otherwise we will set it.
 * @param ds Pointer to the dax state object
 * @param val Value to determine running state
 * @returns zero on success or other error codes if appropriate.
 */
int
dax_set_running(dax_state *ds, uint8_t val) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;
    uint8_t byte;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return result;
    if(h.size > 255) return ERR_2BIG;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return result;
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return result;
    snprintf(runtag, 256, "%s.running", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return result;
    if(val) {
        byte = 0x01;
    } else {
        byte = 0x00;
    }
    result = dax_write_tag(ds, h, &byte);
    return result;
}