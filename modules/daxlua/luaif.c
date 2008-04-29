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
 */

#include <daxlua.h>
#include <opendax.h>
#include <strings.h>

/* Functions to expose to lua interpreter */
/* Adds a tag to the dax tagbase.  Three arguments...
    First - tagname
    Second - tag type
    Third - count
    Returns the handle on success
    Raises an error on failure
*/
static int _dax_tag_add(lua_State *L) {
    char *name, *type;
    int count;
    handle_t result;
    if(lua_gettop(L) != 3) {
        lua_pushnumber(L, -1.0);
        return 1;
    }
    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
    type = (char *)lua_tostring(L, 2); /* Get the datatype from the Lua stack */
    count = (int)lua_tonumber(L, 3); /* Get the count from the Lua stack */
    
    if(name == NULL || type == NULL || count <=0) {
        lua_pushnumber(L, -1.0);
        return 1;
    }
    result = dax_tag_add(name, dax_string_to_type(type), count);
    if( result > 0 ) {
        lua_pushnumber(L, (double)result);
    } else {
        lua_pushnumber(L, -1.0);
    }
    return 1;
}

/* This function figures out what type of data the tag is and translates
*buff appropriately and pushes the value onto the lua stack */
static inline void dax_pushdata(lua_State *L, unsigned int type, void *buff) {
    switch (type) {
        /* Each number has to be cast to the right datatype then dereferenced
           and then cast to double for pushing into the Lua stack */
        case DAX_BYTE:
        case DAX_SINT:
            lua_pushnumber(L, (double)*((u_int8_t *)buff));
            break;
        case DAX_WORD:
        case DAX_UINT:
            lua_pushnumber(L, (double)*((u_int16_t *)buff));
            break;
        case DAX_INT:
            lua_pushnumber(L, (double)*((int16_t *)buff));
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            lua_pushnumber(L, (double)*((u_int32_t *)buff));
            break;
        case DAX_DINT:
            lua_pushnumber(L, (double)*((int32_t *)buff));
            break;
        case DAX_REAL:
            lua_pushnumber(L, (double)*((float *)buff));
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            lua_pushnumber(L, (double)*((u_int64_t *)buff));
            break;
        case DAX_LINT:
            lua_pushnumber(L, (double)*((int64_t *)buff));
            break;
        case DAX_LREAL:
            lua_pushnumber(L, *((double *)buff));
            break;
    }
}

/* Gets the value of the tag.  Accepts one argument, the tagname or handle
and returns the value requested or raises an error on failure */
static int _dax_get(lua_State *L) {
    char *name;
    dax_tag tag;
    int size, n;
    size_t buff_idx, buff_bit;
    void *buff;
    
    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
    /* Make sure that name is not NULL and we get the tagname */
    if( name == NULL) {
        luaL_error(L, "No tagname passed");
    }
    if(dax_tag_byname(name, &tag) ) {
        luaL_error(L, "Unable to retrieve tag - %s", name);
    }
 /* We have to treat Booleans differently */
    if(tag.type == DAX_BOOL) {
        /* Check to see if it's an array */
        if(tag.count > 1 ) {
            size = tag.count / 8 + 1;  //--@#$%@#^$%  Might not need the +1
            buff = alloca(size);
            if(buff == NULL) {
                luaL_error(L, "Unable to allocate buffer size = %d", size);
            }
            dax_tag_read_bits(tag.handle, buff, tag.count);
            buff_idx = buff_bit = 0;
            lua_createtable(L, tag.count, 0);
            for(n = 0; n < tag.count ; n++) {
                if(((u_int8_t *)buff)[buff_idx] & (1 << buff_bit)) { /* If *buff bit is set */
                    lua_pushboolean(L, 1);
                } else {  /* If the bit in the buffer is not set */
                    lua_pushboolean(L, 0);
                }
                lua_rawseti(L, -2, n + 1);
                buff_bit++;
                if(buff_bit == 8) {
                    buff_bit = 0;
                    buff_idx++;
                }
            }
        } else {
            lua_pushboolean(L, dax_tag_read_bit(tag.handle));
        }
 /* Not a boolean */
    } else {
        size = tag.count * TYPESIZE(tag.type) / 8;
        buff = alloca(size);
        if(buff == NULL) {
            luaL_error(L, "Unable to allocate buffer size = %d", size);
        }
        dax_tag_read(tag.handle, 0, buff, size);
        //--DEBUG: printf("tag.count = %d, tag.type = %s\n", tag.count, dax_type_to_string(tag.type));
        
        /* Push the data up to the lua interpreter stack */
        if(tag.count > 1) { /* We need to return a table */
            /* TODO: Since this is all Array stuff we can use 
                     lua_createtable() and the lua_raw*() functions */
            lua_newtable(L);
            for(n = 0; n < tag.count ; n++) {
                lua_pushnumber(L, n + 1); /* Lua likes 1 indexed arrays ?? uahuhauuahhu */
                dax_pushdata(L, tag.type, buff + (TYPESIZE(tag.type) / 8) * n);
                lua_settable(L, -3);
            }
        } else { /* It's a single value */
            dax_pushdata(L, tag.type, buff);
        }
    }
    return 1; /* return number of retvals */
}


static inline void lua_to_dax(lua_State *L, unsigned int type, void *buff, void *mask, int index) {
    lua_Integer x;
    
    switch (type) {
        case DAX_BYTE:
        case DAX_SINT:
            x = lua_tointeger(L, -1) % 256;
            ((u_int8_t *)buff)[index] = x;
            ((u_int8_t *)mask)[index] = 0xFF;
            break;
        case DAX_WORD:
        case DAX_UINT:
            x = lua_tointeger(L, -1) % (2^16);
            ((u_int16_t *)buff)[index] = x;
            ((u_int16_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_INT:
            x = lua_tointeger(L, -1);
            ((int16_t *)buff)[index] = x;
            ((u_int16_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            x = lua_tointeger(L, -1);
            ((u_int32_t *)buff)[index] = x;
            ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_DINT:
            x = lua_tointeger(L, -1);
            ((int32_t *)buff)[index] = x;
            ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_REAL:
            ((float *)buff)[index] = (float)lua_tonumber(L, -1);
            ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            x = lua_tointeger(L, -1);
            ((u_int64_t *)buff)[index] = x;
            ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LINT:
            x = lua_tointeger(L, -1);
            ((int64_t *)buff)[index] = x;
            ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LREAL:
            ((double *)buff)[index] = lua_tonumber(L, -1);
            ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
     }
}

/* Sets a given tag to the given value.  Two arguments...
    First argument is the string or handle for the dax_tag
    Second argument is the value to set the tag too.
    Raises an error on failure
*/
static int _dax_set(lua_State *L) {
    char *name;
    dax_tag tag;
    int size, n, x, index;
    void *buff, *mask;
    
    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
    /* Make sure that name is not NULL and we get the tagname */
    if( name && dax_tag_byname(name, &tag) ) {
        luaL_error(L, "Unable to retrieve tag - %s", name);
    }
    
    if( tag.type == DAX_BOOL ) {
        if( tag.handle % 8 == 0) {
            size = tag.count / 8 + 1;
        } else {
            size = tag.count / 8 + 2;
        }
    } else {
        size = tag.count * TYPESIZE(tag.type) / 8;
    }
    /* Allocate a buffer and a mask */
    buff = alloca(size);
    bzero(buff, size);
    mask = alloca(size);
    bzero(mask, size);
    if(buff == NULL || mask == NULL) {
        luaL_error(L, "Unable to allocate buffer to write tag %s", name);
    }
    
    if(tag.count > 1) { /* The tag is an array */
        /* Check that the second parameter is a table */
        if( ! lua_istable(L, 2) ) { 
            luaL_error(L, "Array needed to set - %s", name);
        }
        /* We're just searching for indexes in the table.  Anything
           other than numerical indexes in the table don't count */
        for(n = 0; n < tag.count ; n++) {
            lua_rawgeti(L, 2, n+1);
            if(lua_isnil(L, -1) == 0) {
                if(tag.type == DAX_BOOL) {
                    /* Handle the boolean */
                    x = (n + tag.handle) % 8;
                    index = ((tag.handle % 8) + n) / 8;
                    if(lua_toboolean(L, -1)) {
                        ((u_int8_t *)buff)[index] |= (1 << (x % 8));
                    } else {  /* If the bit in the buffer is not set */
                        ((u_int8_t *)buff)[index] &= ~(1 << (x % 8));
                    }
                    ((u_int8_t *)mask)[index] |= (1 << (x % 8));
                } else {
                    //Handle the non-boolean
                    lua_to_dax(L, tag.type, buff, mask, n);
                }
            } else {
                lua_pop(L, 1); /* Better pop that nil off the stack */
            }
        }
        /* Write the data to DAX */
        dax_tag_mask_write(tag.handle, 0, buff, mask, size);
    } else { /* Retrieved tag is a single point */
        if(tag.type == DAX_BOOL) {
            dax_tag_write_bit(tag.handle, lua_toboolean(L, -1));
        } else {
            lua_to_dax(L, tag.type, buff, mask, 0);
            dax_tag_write(tag.handle, 0, buff, size);
        }
    }
    return 0; /* return number of retvals */
}

/* Use this function to setup each interpreter with all the right function calls
and libraries */
int setup_interpreter(lua_State *L) {
    lua_pushcfunction(L, _dax_tag_add);
    lua_setglobal(L,"dax_tag_add");
    
    lua_pushcfunction(L, _dax_get);
    lua_setglobal(L,"dax_get");
    
    lua_pushcfunction(L, _dax_set);
    lua_setglobal(L,"dax_set");
    
    
    /* register the libraries that we need*/
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);
    
    return 0; /* I don't think anything returns an error */
}