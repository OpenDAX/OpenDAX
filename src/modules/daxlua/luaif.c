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

 * Main source code file for the Lua script interface functions
 *
 * Many of these functions are duplicated in the daxtest module.
 * At some point they should become a separate Lua package but I
 * don't want to try to learn how to do that just yet.
 */

#include "daxlua.h"
#include <pthread.h>

extern dax_state *ds;


/* This function finds the tag given by *tagname, get's the data from
   the server and puts the result on the top of the Lua stack. */
int
fetch_tag(lua_State *L, tag_handle h)
{
    int result;
    void *data;

    data = malloc(h.size);
    if(data == NULL) {
        return ERR_ALLOC;
    }

    result = dax_read_tag(ds, h, data);

    if(result) {
        free(data);
        return result;
    }

    daxlua_dax_to_lua(L, h, data);

    free(data);
    return 0;
}

/* This function reads the variable from the top of the Lua stack
   and sends it to the opendax tag given by *tagname */
int
send_tag(lua_State *L, tag_handle h)
{
    int result, n;
    char q = 0;
    void *mask, *data;

    data = malloc(h.size);
    if(data == NULL) {
        luaL_error(L, "tag_write() unable to allocate data area");
    }

    mask = malloc(h.size);
    if(mask == NULL) {
        free(data);
        luaL_error(L, "tag_write() unable to allocate mask memory");
    }
    bzero(mask, h.size);

    // TODO: We should be able to convert this to use the libdaxlua library functions instead
    //       and get all this duplication out of here
    result = daxlua_lua_to_dax(L, h, data, mask);
    if(result) {
        free(data);
        free(mask);
        return result;
    }

    /* This checks the mask to determine which function to use
     * to write the data to the server */
    /* TODO: Might want to scan through and find the beginning and end of
     * any changed data and send less through the socket.  */
    for(n = 0; n < h.size && q == 0; n++) {
        if( ((unsigned char *)mask)[n] != 0xFF) {
            q = 1;
        }
    }
    if(q) {
        result = dax_mask_tag(ds, h, data, mask);
    } else {
        result = dax_write_tag(ds, h, data);
    }

    if(result) {
        free(data);
        free(mask);
        return result;
    }

    free(data);
    free(mask);
    return 0;
}



/* TODO: Stuff to add.
 * give the scripts a way to enable and disable each other
 */

/* Use this function to setup each interpreter with all the right function calls
and libraries */
int
setup_interpreter(lua_State *L)
{
    /* This adds the functions that we need from the daxlua library */
    daxlua_register_function(L, "cdt_create");
    daxlua_register_function(L, "tag_add");
    daxlua_register_function(L, "tag_get");
    daxlua_register_function(L, "tag_handle");
    daxlua_register_function(L, "tag_read");
    daxlua_register_function(L, "tag_write");
    daxlua_register_function(L, "log");
    daxlua_register_function(L, "sleep");

    daxlua_set_constants(L);
    /* register the libraries that we need*/
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);

    return 0; /* I don't think anything returns an error */
}
