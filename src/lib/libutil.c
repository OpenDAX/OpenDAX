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

/*!
 * Writes to the modules .status tag
 * @param ds Pointer to the dax state object
 * @param val Value to write to .status tag
 * @returns zero on success or other error codes if appropriate.
 */
int
dax_set_status(dax_state *ds, const char *val) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;
    char *buff;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return result;
    if(h.size > 255) return ERR_2BIG;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return result;
    snprintf(runtag, 256, "%s.status", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return result;
    buff = malloc(h.size);
    if(buff == NULL) return ERR_ALLOC;
    bzero(buff, h.size);
    strncpy(buff, val, h.size);
    result = dax_write_tag(ds, h, buff);
    free(buff);
    return result;
}


/*!
 * Finds the module tags .faulted bit and sets or clears it depending
 * on the boolean state of val.  If val is zero then we will clear
 * the faulted bit, otherwise we will set it.
 * @param ds Pointer to the dax state object
 * @param val Value to determine faulted state
 * @returns zero on success or other error codes if appropriate.
 */
int
dax_set_faulted(dax_state *ds, uint8_t val) {
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
    snprintf(runtag, 256, "%s.faulted", tagname);
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


/*!
 * Default run callback function.  Retrieves the handle
 * to the .run member of the modules tag and clears it.
 * Also sets the .running bit in the module tag
 * @param ds Pointer to the dax state object
 * @param ud unused userdata
 * @returns zero on success or other error codes if appropriate.
 */

void
dax_default_run(dax_state *ds, void *ud) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;
    uint8_t byte;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return;
    if(h.size > 255) return;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return;
    snprintf(runtag, 256, "%s.run", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return;
    byte = 0x00;
    result = dax_write_tag(ds, h, &byte);
    dax_set_running(ds, 1);
}

/*!
 * Default stop callback function.  Retrieves the handle
 * to the .stop member of the modules tag and clears it
 * Also clears the .running bit in the module tag
 * @param ds Pointer to the dax state object
 * @param ud unused userdata
 * @returns zero on success or other error codes if appropriate.
 */
void
dax_default_stop(dax_state *ds, void *ud) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;
    uint8_t byte;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return;
    if(h.size > 255) return;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return;
    snprintf(runtag, 256, "%s.stop", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return;
    byte = 0x00;
    result = dax_write_tag(ds, h, &byte);
    dax_set_running(ds, 0);
}

/*!
 * Default kill callback function.  Retrieves the handle
 * to the .kill member of the modules tag and clears it
 * @param ds Pointer to the dax state object
 * @param ud unused userdata
 * @returns zero on success or other error codes if appropriate.
 */
void
dax_default_kill(dax_state *ds, void *ud) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;
    uint8_t byte;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return;
    if(h.size > 255) return;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return;
    snprintf(runtag, 256, "%s.kill", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return;
    byte = 0x00;
    result = dax_write_tag(ds, h, &byte);
}

/*!
 * Default kill callback function.  Retrieves the handle
 * to the .kill member of the modules tag and clears it
 * @param ds Pointer to the dax state object
 * @returns zero on success or other error codes if appropriate.
 */
int
dax_set_default_callbacks(dax_state *ds) {
    int result;
    result = dax_set_run_callback(ds, dax_default_run);
    if(result) return result;
    result = dax_set_stop_callback(ds, dax_default_stop);
    if(result) return result;
    result = dax_set_kill_callback(ds, dax_default_kill);
    if(result) return result;
    return 0;
}



/*!
 * Finds the module tags .run bit and adds a set event to that
 * bit with the given function as the callback for the event.
 * @param ds Pointer to the dax state object
 * @param f Function to use as the callback
 * @returns zero on success or other error codes if appropriate.
 */
int
dax_set_run_callback(dax_state *ds, void f(dax_state *, void *)) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return result;
    if(h.size > 255) return ERR_2BIG;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return result;
    snprintf(runtag, 256, "%s.run", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return result;

    result = dax_event_add(ds, &h, EVENT_SET, NULL, NULL,f, NULL, NULL);
    return result;
}

/*!
 * Finds the module tags .stop bit and adds a set event to that
 * bit with the given function as the callback for the event.
 * @param ds Pointer to the dax state object
 * @param f Function to use as the callback
 * @returns zero on success or other error codes if appropriate.
 */
int
dax_set_stop_callback(dax_state *ds, void f(dax_state *, void *)) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return result;
    if(h.size > 255) return ERR_2BIG;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return result;
    snprintf(runtag, 256, "%s.stop", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return result;

    result = dax_event_add(ds, &h, EVENT_SET, NULL, NULL,f, NULL, NULL);
    return result;
}


/*!
 * Finds the module tags .kill bit and adds a set event to that
 * bit with the given function as the callback for the event.
 * @param ds Pointer to the dax state object
 * @param f Function to use as the callback
 * @returns zero on success or other error codes if appropriate.
 */
int
dax_set_kill_callback(dax_state *ds, void f(dax_state *, void *)) {
    tag_handle h;
    char tagname[DAX_TAGNAME_SIZE+1];
    char runtag[DAX_TAGNAME_SIZE+32];
    int result;

    /* Get a handle to our module tagname */
    result = dax_tag_handle(ds, &h, _MY_TAGNAME, 0);
    if(result) return result;
    if(h.size > 255) return ERR_2BIG;
    /* Read the tagname */
    result = dax_read_tag(ds, h, tagname);
    if(result) return result;
    snprintf(runtag, 256, "%s.kill", tagname);
    result = dax_tag_handle(ds, &h, runtag, 0);
    if(result) return result;

    result = dax_event_add(ds, &h, EVENT_SET, NULL, NULL,f, NULL, NULL);
    return result;
}

/*!
 * Returns the maximum number of bytes that can be returned in
 * the data area for a single tag read.  Tags can be larger than
 * this but they must be read in parts.
 * @returns the requested size
 */
int
dax_get_max_tag_msg_size(void) {
    return MSG_DATA_SIZE;
}