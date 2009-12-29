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

#include <daxlua.h>
#include <pthread.h>

extern pthread_mutex_t daxmutex;

int
daxlua_init(void)
{
    pthread_mutex_init(&daxmutex, NULL);
    return 0;
}

/* Wrapper function for the cdt creation functions */
/* The Lua function should be passed two arguments.  The
 * first is the name of the datatype and the second is
 * a table of tables that represent all the members of the
 * compound datatype.  It should be formatted like this...
 * members = {{"Name", "DataType", count},
 *            {"AnotherNmae", "DataType", count}} */
static int
_cdt_create(lua_State *L)
{
    int count, n = 1, result;
    dax_cdt *cdt;
    char *name, *type, *cdt_name;
    
    if(lua_gettop(L) != 2) {
        luaL_error(L, "Wrong number of arguments to cdt_create()");
    }
    
    cdt_name = (char *)lua_tostring(L, 1);
    cdt = dax_cdt_new(cdt_name, NULL);
    
    if(cdt == 0) {
        luaL_error(L, "Unable to create datatype %s", lua_tostring(L, 1));
    }
    
    if(! lua_istable(L, 2) ) {
        luaL_error(L, "Should pass a table to tag_handle_test()");
    }
    
    while(1) {
        /* Get the table from the argument */
        lua_rawgeti(L, 2, n++);
        if(lua_isnil(L, -1)) break;
        lua_rawgeti(L, 3, 1);
        name = (char *)lua_tostring(L, -1);
        lua_rawgeti(L, 3, 2);
        type = (char *)lua_tostring(L, -1);
        lua_rawgeti(L, 3, 3);
        count = lua_tointeger(L, -1);
        lua_pop(L, 4);
        
        pthread_mutex_lock(&daxmutex);
        result = dax_cdt_member(cdt, name, dax_string_to_type(type), count);
        pthread_mutex_unlock(&daxmutex);
        if(result) {
            dax_cdt_free(cdt);
            luaL_error(L, "Unable to add member %s", name);
        }
    }
    pthread_mutex_lock(&daxmutex);
    result = dax_cdt_create(cdt, NULL);
    pthread_mutex_unlock(&daxmutex);
    
    if(result) {
        luaL_error(L, "Unable to create datatype %s", cdt_name);
    }
    
    lua_pushinteger(L, result);
    return 1;
}

/* Functions to expose to lua interpreter */
/* Adds a tag to the dax tagbase.  Three arguments...
    First - tagname
    Second - tag type
    Third - count
    Returns the handle on success
    Raises an error on failure
*/
static int
_dax_tag_add(lua_State *L)
{
    char *name, *datatype;
    int count;
    tag_type type;
    tag_index result;
    Handle h;
    if(lua_gettop(L) != 3) {
        lua_pushnumber(L, -1.0);
        return 1;
    }
    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
    datatype = (char *)lua_tostring(L, 2); /* Get the datatype from the Lua stack */
    count = (int)lua_tonumber(L, 3);   /* Get the count from the Lua stack */
    
    if(name == NULL || datatype == NULL || count <=0) {
        luaL_error(L, "Bad argument passed to dax_tag_add()");
    }
    
    pthread_mutex_lock(&daxmutex);
    type = dax_string_to_type(datatype);
    if(type == 0) {
        pthread_mutex_unlock(&daxmutex);
        luaL_error(L, "Unrecognized datatype %s\n", datatype);
    }
    result = dax_tag_add(&h, name, type, count);
    printf("dax_tag_add() returned %d\n", result);
    pthread_mutex_unlock(&daxmutex);
    
    if( result == 0 ) {
        lua_pushnumber(L, (double)h.index);
    } else {
        luaL_error(L, "Unable to Create Tag %s", name);
    }
    return 1;
}


/* These are the main data transfer functions.  These are wrappers for
 * the dax_read/write/mask function. */
static int
_dax_read(lua_State *L) {
    char *name;
    int count, result;
    Handle h;
    void *data;
    
    /* TODO: Might allow one argument and then set count = 0 */
    if(lua_gettop(L) != 2) {
        luaL_error(L, "Wrong number of arguments passed to tag_read()");
    }
    name = (char *)lua_tostring(L, 1);
    count = lua_tointeger(L, 2);
    
    pthread_mutex_lock(&daxmutex);
    result = dax_tag_handle(&h, name, count);
    if(result) {
        pthread_mutex_unlock(&daxmutex);
        luaL_error(L, "dax_tag_handle() returned %d", result);
    }
    data = malloc(h.size);
    if(data == NULL) {
        pthread_mutex_unlock(&daxmutex);
        luaL_error(L, "tag_read() unable to allocate data area");
    }
    
    result = dax_read_tag(h, data);
    
    if(result) {
        pthread_mutex_unlock(&daxmutex);
        free(data);
        luaL_error(L, "dax_read_tag() returned %d", result);
    }
    /* This function figures all the tag data out and pushes the right
     * thing onto the top of the Lua stack */
    tag_dax_to_lua(L, h, data);
    pthread_mutex_unlock(&daxmutex);

    free(data);
    return 1;
}

static int
_dax_write(lua_State *L) {
    char *name;
    char q = 0;
    int result, n;
    Handle h;
    void *data, *mask;
    
    if(lua_gettop(L) != 2) {
        luaL_error(L, "Wrong number of arguments passed to tag_write()");
    }
    name = (char *)lua_tostring(L, 1);
    pthread_mutex_lock(&daxmutex);
    result = dax_tag_handle(&h, name, 0);
    if(result) {
        pthread_mutex_unlock(&daxmutex);
        luaL_error(L, "dax_tag_handle() returned %d", result);
    }
    
    data = malloc(h.size);
    if(data == NULL) {
        pthread_mutex_unlock(&daxmutex);
        luaL_error(L, "tag_write() unable to allocate data area");
    }

    mask = malloc(h.size);
    if(mask == NULL) {
        pthread_mutex_unlock(&daxmutex);
        free(data);
        luaL_error(L, "tag_write() unable to allocate mask memory");
    }
    bzero(mask, h.size);
    
    result = tag_lua_to_dax(L, h, data, mask);
    if(result) {
        pthread_mutex_unlock(&daxmutex);
        free(data);
        free(mask);
        lua_error(L); /* The error message should already be on top of the stack */
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
        result = dax_mask_tag(h, data, mask);
    } else {
        result = dax_write_tag(h, data);
    }
    pthread_mutex_unlock(&daxmutex);
    
    if(result) {
        free(data);
        free(mask);
        luaL_error(L, "dax_write/mask_tag() returned %d", result);
    }

    free(data);
    free(mask);
    return 1;
}

/* The following functions take care of reading and writing tag data */

/* This is just a convienience since I need to pass multiple pieces
 * of data back from the cdt_iter callback. */
struct iter_udata {
    lua_State *L;
    void *data;
    void *mask;
    int error;
};

/* This function figures out what type of data the tag is and translates
 * buff appropriately and pushes the value onto the lua stack */
static inline void
_read_to_stack(lua_State *L, tag_type type, void *buff)
{
    switch (type) {
        /* Each number has to be cast to the right datatype then dereferenced
           and then cast to double for pushing into the Lua stack */
        case DAX_BYTE:
            lua_pushnumber(L, (double)*((dax_byte *)buff));
            break;
        case DAX_SINT:
            lua_pushnumber(L, (double)*((dax_sint *)buff));
            break;
        case DAX_WORD:
        case DAX_UINT:
            lua_pushnumber(L, (double)*((dax_uint *)buff));
            break;
        case DAX_INT:
            lua_pushnumber(L, (double)*((dax_int *)buff));
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            lua_pushnumber(L, (double)*((dax_udint *)buff));
            break;
        case DAX_DINT:
            lua_pushnumber(L, (double)*((dax_dint *)buff));
            break;
        case DAX_REAL:
            lua_pushnumber(L, (double)*((dax_real *)buff));
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            lua_pushnumber(L, (double)*((dax_ulint *)buff));
            break;
        case DAX_LINT:
            lua_pushnumber(L, (double)*((dax_lint *)buff));
            break;
        case DAX_LREAL:
            lua_pushnumber(L, *((dax_lreal *)buff));
            break;
    }
}


static void
_push_base_datatype(lua_State *L, cdt_iter tag, void *data)
{
    int n, bit;
    
    /* We have to treat Booleans differently */
    if(tag.type == DAX_BOOL) {
        /* Check to see if it's an array */
        if(tag.count > 1 ) {
            lua_createtable(L, tag.count, 0);
            for(n = 0; n < tag.count ; n++) {
                bit = n + tag.bit;
                if(((u_int8_t *)data)[bit/8] & (1 << (bit % 8))) { /* If *buff bit is set */
                    lua_pushboolean(L, 1);
                } else {  /* If the bit in the buffer is not set */
                    lua_pushboolean(L, 0);
                }
                lua_rawseti(L, -2, n + 1);
            }
        } else {
            if(((u_int8_t *)data)[tag.bit/8] & (1 << (tag.bit % 8))) { /* If *buff bit is set */
                lua_pushboolean(L, 1);
            } else {  /* If the bit in the buffer is not set */
                lua_pushboolean(L, 0);
            }
        }
    } else { /* Not a boolean */
        /* Push the data up to the lua interpreter stack */
        if(tag.count > 1) { /* We need to return a table */
            lua_createtable(L, tag.count, 0);
            for(n = 0; n < tag.count ; n++) {
                _read_to_stack(L, tag.type, data + (TYPESIZE(tag.type) / 8) * n);
                lua_rawseti(L, -2, n + 1); /* Lua likes 1 indexed arrays */
            }
        } else { /* It's a single value */
            _read_to_stack(L, tag.type, data);
        }
    }
}

/* When the dax_cdt_iter() function that refers to this function returns
 * there should be a table at the top of the stack that represents the
 * contents of the member given by 'member' */
static void
read_callback(cdt_iter member, void *udata)
{
    lua_State *L = ((struct iter_udata *)udata)->L;
    unsigned char *data = ((struct iter_udata *)udata)->data;
    int offset, n;
    struct iter_udata newdata;

    lua_pushstring(L, member.name);
            
    if(IS_CUSTOM(member.type)) {
        lua_newtable(L);
        newdata.L = L;
        newdata.mask = NULL;
        newdata.error = 0;
        
        if(member.count > 1) {
            for(n = 0;n < member.count; n++) {
                lua_newtable(L);
                offset = member.byte + (n * dax_get_typesize(member.type));
                newdata.data = data + offset;
                dax_cdt_iter(member.type, &newdata , read_callback);
                lua_rawseti(L, -2, n + 1);
            }
        } else {
            newdata.data = data + member.byte;
            dax_cdt_iter(member.type, &newdata, read_callback);
        }
    } else {
        _push_base_datatype(L, member, data + member.byte);
    }
    lua_rawset(L, -3);
}

/* This is the top level function for taking the data that is is in *data,
 * iterating through the tag give by handle 'h' and storing that information
 * into a Lua variable on the top of the Lua stack. */
void
tag_dax_to_lua(lua_State *L, Handle h, void *data)
{
    cdt_iter tag;
    struct iter_udata udata;
    int offset, n;
    
    udata.L = L;
    udata.data = data;
    udata.mask = NULL;
    udata.error = 0;

    if(IS_CUSTOM(h.type)) {
        lua_newtable(L);
        
        if(h.count > 1) {
            for(n = 0; n < h.count; n++) {
                lua_newtable(L);    
                offset = n * dax_get_typesize(h.type);
                udata.data = (char *)data + offset;
                dax_cdt_iter(h.type, &udata, read_callback);
                lua_rawseti(L, -2, n+1);
            }
        } else {
            dax_cdt_iter(h.type, &udata, read_callback);
        }
    } else {
        tag.count = h.count;
        tag.type = h.type;
        tag.byte = 0;
        tag.bit = 0;
        _push_base_datatype(L, tag, data);
    } 
}

/* Here begin the tag writing functions.  */

/* Takes the Lua value at the top of the stack, converts it and places it
 * in the proper place in data.  The mask is set as well.  */
static inline void
_write_from_stack(lua_State *L, unsigned int type, void *data, void *mask, int index)
{
    lua_Integer x;
    
    assert(mask != NULL);
    switch (type) {
        case DAX_BYTE:
            x = lua_tointeger(L, -1) % 256;
            ((dax_byte *)data)[index] = x;
            ((dax_byte *)mask)[index] = 0xFF;
            break;
        case DAX_SINT:
            x = lua_tointeger(L, -1) % 256;
            ((dax_sint *)data)[index] = x;
            ((dax_sint *)mask)[index] = 0xFF;
            break;
        case DAX_WORD:
        case DAX_UINT:
            x = lua_tointeger(L, -1);
            ((dax_uint *)data)[index] = x;
            ((dax_uint *)mask)[index] = 0xFFFF;
            break;
        case DAX_INT:
            x = lua_tointeger(L, -1);
            ((dax_int *)data)[index] = x;
            ((dax_int *)mask)[index] = 0xFFFF;
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            x = lua_tointeger(L, -1);
            ((dax_udint *)data)[index] = x;
            ((dax_udint *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_DINT:
            x = lua_tointeger(L, -1);
            ((dax_dint *)data)[index] = x;
            ((dax_dint *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_REAL:
            ((dax_real *)data)[index] = (dax_real)lua_tonumber(L, -1);
            ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            x = lua_tointeger(L, -1);
            ((dax_ulint *)data)[index] = x;
            ((dax_ulint *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LINT:
            x = lua_tointeger(L, -1);
            ((dax_lint *)data)[index] = x;
            ((dax_lint *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LREAL:
            ((dax_lreal *)data)[index] = lua_tonumber(L, -1);
            ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
    }
}

/* This function reads the variable from the top of the Lua stack
   and sends it to the opendax tag given by *tagname */
int
_pop_base_datatype(lua_State *L, cdt_iter tag, void *data, void *mask)
{
    int n, bit;
    
    //printf("_pop_base_datatype() called with *data = %p\n", data);
    if(tag.count > 1) { /* The tag is an array */
        /* Check that the second parameter is a table */
        if( ! lua_istable(L, -1) ) {
            lua_pushfstring(L, "Table needed to set - %s", tag.name);
            return -1;
        }
        /* We're just searching for indexes in the table.  Anything
         other than numerical indexes in the table don't count */
        for(n = 0; n < tag.count; n++) {
            lua_rawgeti(L, -1, n + 1);
            if(! lua_isnil(L, -1)) {
                if(tag.type == DAX_BOOL) {
                    /* Handle the boolean */
                    bit = n + tag.bit;
                    if(lua_toboolean(L, -1)) {
                        ((u_int8_t *)data)[bit/8] |= (1 << (bit % 8));
                    } else {  /* If the bit in the buffer is not set */
                        ((u_int8_t *)data)[bit/8] &= ~(1 << (bit % 8));
                    }
                    ((u_int8_t *)mask)[bit/8] |= (1 << (bit % 8));
                } else {
                    /* Handle the non-boolean */
                    _write_from_stack(L, tag.type, data, mask, n);
                }
            }
            lua_pop(L, 1);
        }
    } else { /* Retrieved tag is a single point */
        if(tag.type == DAX_BOOL) {
            bit = tag.bit;
//            printf("tag.bit = %d \n", tag.bit);
//            printf("Before Data[%d] = 0x%X\n", bit/8, ((u_int8_t *)data)[bit/8]);
//            printf("Before Mask[%d] = 0x%X\n", bit/8, ((u_int8_t *)mask)[bit/8]);
            if(lua_toboolean(L, -1)) {
//                printf("set to TRUE\n");
                ((u_int8_t *)data)[bit/8] |= (1 << (bit % 8));
            } else {  /* If the bit in the buffer is not set */
//                printf("set to FALSE\n");
                ((u_int8_t *)data)[bit/8] &= ~(1 << (bit % 8));
            }
            ((u_int8_t *)mask)[bit/8] |= (1 << (bit % 8));
//            printf("After Data[%d] = 0x%X\n", bit/8, ((u_int8_t *)data)[bit/8]);
//            printf("After Mask[%d] = 0x%X\n", bit/8, ((u_int8_t *)mask)[bit/8]);
            
        } else {
            _write_from_stack(L, tag.type, data, mask, 0);
        }
    }
    return 0;
}

/* This is the recursive callback function that is passed to
 * dax_cdt_iter() to handle the compound data types.  If the
 * type has nested cdt's then this function will call itself */
static void
write_callback(cdt_iter member, void *udata)
{
    struct iter_udata newdata;
    
    lua_State *L = ((struct iter_udata *)udata)->L;
    unsigned char *data = ((struct iter_udata *)udata)->data;
    unsigned char *mask = ((struct iter_udata *)udata)->mask;
    int offset, n, result = 0;
    
    if(IS_CUSTOM(member.type)) {
        newdata.L = L;
        newdata.error = 0;
        lua_pushstring(L, member.name);
        lua_rawget(L, -2);
        if(member.count > 1) {
            for(n = 0;n < member.count; n++) {
                offset = member.byte + (n * dax_get_typesize(member.type));
                newdata.data = (char *)data + offset;
                newdata.mask = (char *)mask + offset;
                if( ! lua_istable(L, -1) ) {
                    lua_pushfstring(L, "Table needed to set - %s", member.name);
                    ((struct iter_udata *)udata)->error = -1;
                    return;
                }
                lua_rawgeti(L, -1, n+1);
                if(! lua_isnil(L, -1)) {
                    if( ! lua_istable(L, -1) ) {
                        lua_pop(L, 1);
                        lua_pushfstring(L, "Table needed to set - %s", member.name);
                        ((struct iter_udata *)udata)->error = -1;
                        return;
                    }
                    dax_cdt_iter(member.type, &newdata , write_callback);
                }
                lua_pop(L, 1);
                if(newdata.error) {
                    ((struct iter_udata *)udata)->error = newdata.error;
                    return;
                }
            }
        } else {
            newdata.data = (char *)data + member.byte;
            newdata.mask = (char *)mask + member.byte;
            if(! lua_isnil(L, -1)) {
                if( ! lua_istable(L, -1) ) {
                    lua_pushfstring(L, "Table needed to set - %s", member.name);
                    ((struct iter_udata *)udata)->error = -1;
                    return;
                }
                dax_cdt_iter(member.type, &newdata, write_callback);
            }
            if(newdata.error) {
                ((struct iter_udata *)udata)->error = newdata.error;
                return;
            }
        }
        lua_pop(L, 1);
    } else {
        lua_pushstring(L, member.name);
        lua_rawget(L, -2);
        if(! lua_isnil(L, -1)) {
            result = _pop_base_datatype(L, member, data + member.byte, mask + member.byte);
        }
        lua_pop(L, 1);
    }
    if(result) {
        ((struct iter_udata *)udata)->error = result;
    }
}

/* This function takes care of the top level of the tag.  If the tag
 * is a simple base datatype tag then the _pop_base_datatype() function
 * is called directly and write is complete.  If the tag is a compound
 * datatype then the top level is taken care of and then it's turned
 * over to the recursive write_callback() function through the
 * cdt iterator. */
int
tag_lua_to_dax(lua_State *L, Handle h, void* data, void *mask){
    cdt_iter tag;
    struct iter_udata udata;
    int n, offset;
    
    if(IS_CUSTOM(h.type)) {
        udata.L = L;
        udata.error = 0;
        if(h.count > 1) {
            for(n = 0; n < h.count; n++) {
                offset = n * dax_get_typesize(h.type);
                udata.data = (char *)data + offset;
                udata.mask = (char *)mask + offset;
                if( ! lua_istable(L, -1) ) {
                    lua_pushfstring(L, "Table needed to set - %s", tag.name);
                    return -1;
                }
                lua_rawgeti(L, -1, n+1);
                if(! lua_isnil(L, -1)) {
                    if( ! lua_istable(L, -1) ) {
                        lua_pop(L, 1);
                        lua_pushstring(L, "Table needed to set tag");
                        return -1;
                    }
                    dax_cdt_iter(h.type, &udata , write_callback);
                }
                lua_pop(L, 1);
                if(udata.error) {
                    return udata.error;
                }
            }
        } else {
            udata.data = data;
            udata.mask = mask;
            if(! lua_isnil(L, -1)) {
                if( ! lua_istable(L, -1) ) {
                    lua_pushstring(L, "Table needed to set Tag");
                    return -1;
                }
                dax_cdt_iter(h.type, &udata, write_callback);
            }
            if(udata.error) {
                return udata.error;
            }
        }
    } else {
        tag.count = h.count;
        tag.type = h.type;
        tag.byte = 0;
        tag.bit = 0;
        return _pop_base_datatype(L, tag, data, mask);
    }
    return 0;
}


///* This function figures out what type of data the tag is and translates
//*buff appropriately and pushes the value onto the lua stack */
//static inline void
//_dax_pushdata(lua_State *L, unsigned int type, void *buff)
//{
//    switch (type) {
//        /* Each number has to be cast to the right datatype then dereferenced
//           and then cast to double for pushing into the Lua stack */
//        case DAX_BYTE:
//        case DAX_SINT:
//            lua_pushnumber(L, (double)*((dax_sint *)buff));
//            break;
//        case DAX_WORD:
//        case DAX_UINT:
//            lua_pushnumber(L, (double)*((dax_uint *)buff));
//            break;
//        case DAX_INT:
//            lua_pushnumber(L, (double)*((dax_int *)buff));
//            break;
//        case DAX_DWORD:
//        case DAX_UDINT:
//        case DAX_TIME:
//            lua_pushnumber(L, (double)*((dax_udint *)buff));
//            break;
//        case DAX_DINT:
//            lua_pushnumber(L, (double)*((dax_dint *)buff));
//            break;
//        case DAX_REAL:
//            lua_pushnumber(L, (double)*((dax_real *)buff));
//            break;
//        case DAX_LWORD:
//        case DAX_ULINT:
//            lua_pushnumber(L, (double)*((dax_ulint *)buff));
//            break;
//        case DAX_LINT:
//            lua_pushnumber(L, (double)*((dax_lint *)buff));
//            break;
//        case DAX_LREAL:
//            lua_pushnumber(L, *((dax_lreal *)buff));
//            break;
//    }
//}
//
///* Takes the Lua value at the top of the stack, converts it and places it
//   in the proper place in buff.  If mask is not NULL it will be set as well */
//static inline void
//_lua_to_dax(lua_State *L, unsigned int type, void *buff, void *mask, int index)
//{
//    lua_Integer x;
//    
//    switch (type) {
//        case DAX_BYTE:
//        case DAX_SINT:
//            x = lua_tointeger(L, -1) % 256;
//            ((dax_sint *)buff)[index] = x;
//            if(mask) ((dax_sint *)mask)[index] = 0xFF;
//            break;
//        case DAX_WORD:
//        case DAX_UINT:
//            x = lua_tointeger(L, -1);
//            ((dax_uint *)buff)[index] = x;
//            if(mask) ((dax_uint *)mask)[index] = 0xFFFF;
//            break;
//        case DAX_INT:
//            x = lua_tointeger(L, -1);
//            ((dax_int *)buff)[index] = x;
//            if(mask) ((dax_int *)mask)[index] = 0xFFFF;
//            break;
//        case DAX_DWORD:
//        case DAX_UDINT:
//        case DAX_TIME:
//            x = lua_tointeger(L, -1);
//            ((dax_udint *)buff)[index] = x;
//            if(mask) ((dax_udint *)mask)[index] = 0xFFFFFFFF;
//            break;
//        case DAX_DINT:
//            x = lua_tointeger(L, -1);
//            ((dax_dint *)buff)[index] = x;
//            if(mask) ((dax_dint *)mask)[index] = 0xFFFFFFFF;
//            break;
//        case DAX_REAL:
//            ((dax_real *)buff)[index] = (dax_real)lua_tonumber(L, -1);
//            if(mask) ((dax_real *)mask)[index] = 0xFFFFFFFF;
//            break;
//        case DAX_LWORD:
//        case DAX_ULINT:
//            x = lua_tointeger(L, -1);
//            ((dax_ulint *)buff)[index] = x;
//            if(mask) ((dax_ulint *)mask)[index] = DAX_64_ONES;
//            break;
//        case DAX_LINT:
//            x = lua_tointeger(L, -1);
//            ((dax_lint *)buff)[index] = x;
//            if(mask) ((dax_lint *)mask)[index] = DAX_64_ONES;
//            break;
//        case DAX_LREAL:
//            ((dax_lreal *)buff)[index] = lua_tonumber(L, -1);
//            if(mask) ((dax_lreal *)mask)[index] = DAX_64_ONES;
//            break;
//    }
//}

/* This function finds the tag given by *tagname, get's the data from
   the server and puts the result on the top of the Lua stack. */
int
fetch_tag(lua_State *L, Handle h)
{
    int result;
    void *data;
    
    data = malloc(h.size);
    if(data == NULL) {
        return ERR_ALLOC;
    }
    
    result = dax_read_tag(h, data);

    if(result) {
        free(data);
        return result;
    }
    /* This function figures all the tag data out and pushes the right
     * thing onto the top of the Lua stack */
    tag_dax_to_lua(L, h, data);
    
    free(data);   
    return 0;
}
//    dax_tag tag;
//    int size, n, result;
//    void *buff;
//
//    if( (result = dax_tag_byname(&tag, tagname)) ) {
//        luaL_error(L, "Unable to retrieve tag - %s, error %d", tagname, result);
//    }
//    
//    if(tag.type == DAX_BOOL) {
//        size = tag.count / 8 +1;
//    } else {
//        size = tag.count * TYPESIZE(tag.type) / 8;
//    }
//    buff = malloc(size);
//    if(buff == NULL) {
//        luaL_error(L, "Unable to allocate buffer size = %d", size);
//    }
//    //result = dax_read_tag(tag.idx, 0, buff, tag.count, tag.type);
//    assert(0); /* This assert is because we have comented out the tag reading */
//    
//    if(result) {
//        free(buff);
//        luaL_error(L, "Unable to read tag - %s", tag.name);
//    }
//    
//    /* We have to treat Booleans differently */
//    if(tag.type == DAX_BOOL) {
//        /* Check to see if it's an array */
//        if(tag.count > 1 ) {
//            lua_createtable(L, tag.count, 0);
//            for(n = 0; n < tag.count ; n++) {
//                if(((u_int8_t *)buff)[n/8] & (1 << n%8)) { /* If *buff bit is set */
//                    lua_pushboolean(L, 1);
//                } else {  /* If the bit in the buffer is not set */
//                    lua_pushboolean(L, 0);
//                }
//                lua_rawseti(L, -2, n + 1);
//            }
//        } else {
//            lua_pushboolean(L, ((char *)buff)[0]);
//        }
//        /* Not a boolean */
//    } else {
//        /* Push the data up to the lua interpreter stack */
//        if(tag.count > 1) { /* We need to return a table */
//            lua_createtable(L, tag.count, 0);
//            for(n = 0; n < tag.count ; n++) {
//                _dax_pushdata(L, tag.type, buff + (TYPESIZE(tag.type) / 8) * n);
//                lua_rawseti(L, -2, n + 1); /* Lua likes 1 indexed arrays */
//            }
//        } else { /* It's a single value */
//            _dax_pushdata(L, tag.type, buff);
//        }
//    }
//    
//    free(buff);
//    return 0;
//}

///* Gets the value(s) of the tag.  Accepts one argument, the tagname
//and returns the value requested or raises an error on failure */
//static int
//_dax_read(lua_State *L)
//{
//    char *name;
//    
//    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
//    /* Make sure that name is not NULL and we get the tagname */
//    if( name == NULL) {
//        luaL_error(L, "No tagname passed");
//    }
//    pthread_mutex_lock(&daxmutex);
//    fetch_tag(L, name);
//    pthread_mutex_unlock(&daxmutex);
//
//    return 1; /* return number of retvals */
//}
//

/* This function reads the variable from the top of the Lua stack
   and sends it to the opendax tag given by *tagname */
int
send_tag(lua_State *L, Handle h)
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
    
    result = tag_lua_to_dax(L, h, data, mask);
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
        result = dax_mask_tag(h, data, mask);
    } else {
        result = dax_write_tag(h, data);
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
//    dax_tag tag;
//    int size, n;
//    void *buff, *mask;
//    
//    if( dax_tag_byname(&tag, tagname) ) {
//        luaL_error(L, "Unable to retrieve tag - %s", tagname);
//    }
//    if( tag.type == DAX_BOOL ) {
//        size = tag.count / 8 + 1;
//    } else {
//        size = tag.count * TYPESIZE(tag.type) / 8;
//    }
//    /* Allocate a buffer and a mask */
//    buff = alloca(size);
//    bzero(buff, size);
//    mask = alloca(size);
//    bzero(mask, size);
//    if(buff == NULL || mask == NULL) {
//        luaL_error(L, "Unable to allocate buffer to write tag %s", tagname);
//    }
//    
//    if(tag.count > 1) { /* The tag is an array */
//        /* Check that the second parameter is a table */
//        if( ! lua_istable(L, -1) ) { 
//            luaL_error(L, "Table needed to set - %s", tagname);
//        }
//        /* We're just searching for indexes in the table.  Anything
//         other than numerical indexes in the table don't count */
//        for(n = 0; n < tag.count; n++) {
//            lua_rawgeti(L, -1, n+1);
//            if(lua_isnil(L, -1)) {
//                lua_pop(L, 1); /* Better pop that nil off the stack */
//            } else { /* Ok we have something let's deal with it */
//                if(tag.type == DAX_BOOL) {
//                    /* Handle the boolean */
//                    if(lua_toboolean(L, -1)) {
//                        ((u_int8_t *)buff)[n/8] |= (1 << (n % 8));
//                    } else {  /* If the bit in the buffer is not set */
//                        ((u_int8_t *)buff)[n/8] &= ~(1 << (n % 8));
//                    }
//                    ((u_int8_t *)mask)[n/8] |= (1 << (n % 8));
//                } else {
//                    /* Handle the non-boolean */
//                    _lua_to_dax(L, tag.type, buff, mask, n);
//                }
//            }
//            lua_pop(L, 1);
//        }
//        /* Write the data to DAX */
//        //--dax_mask_tag(tag.idx, 0, buff, mask, tag.count, tag.type);
//        assert(0); /* This is because we have commented out the above mask function */
//    } else { /* Retrieved tag is a single point */
//        if(tag.type == DAX_BOOL) {
//            ((char *)buff)[0] = lua_toboolean(L, -1);
//        } else {
//            _lua_to_dax(L, tag.type, buff, mask, 0);
//        }
//        //--dax_write_tag(tag.idx, 0, buff, tag.count, tag.type);
//        assert(0); /* This is because we have commented out the above write function */
//    }
//    return 0;
//}

///* Sets a given tag to the given value.  Two arguments...
// * First argument is the string for the dax_tag
// * Second argument is the value to set the tag too.
// * Raises an error on failure
// */
//static int
//_dax_write(lua_State *L)
//{
//    char *name;
//        
//    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
//    /* Make sure that name is not NULL and we get the tagname */
//    if(name) {
//        pthread_mutex_lock(&daxmutex);
//        send_tag(L, name);
//        pthread_mutex_unlock(&daxmutex);
//    } else {
//        luaL_error(L, "No Name Given to Write");
//        return -1;
//    }
//    
//    return 0; /* return number of retvals */
//}

static int
_add_global(char *script, char *varname, unsigned char mode)
{
    global_t *glo;
    script_t *scr;
    Handle h;
    int result;
    
    /* This would indicate that it's a dax tag */
    if(mode != MODE_STATIC) {
        result = dax_tag_handle(&h, varname, 0);
        if(result) return result;
    }
    
    scr = get_script_name(script);
    if(scr == NULL) return -1;
    
    glo = malloc(sizeof(global_t));
    if(glo) {
        glo->name = strdup( varname );
        glo->mode = mode;
        glo->ref = LUA_NOREF;
        glo->handle = h;
        glo->next = scr->globals;
        scr->globals = glo;
    } else {
        return ERR_ALLOC;
    }
    return 0;
}

/* Adds a tag to the *globals list for registration
   Three arguments.  First is name of the script
   second is the name of the tag and the third is
   is the read/write mode ("R", "W" or "RW") */
/* TODO: should this be allowed in a normal script?  I guess
   I don't want any limits but couldn't the scriptname be assumed
   if run from a normal script or given if meant for another script? */
/* TODO: Should we add the ability to remove these from the list? */
static int
_register_tag(lua_State *L)
{
    char *script, *varname, *modestring;
    unsigned char mode = 0;
    int n;
    
    script = (char *)lua_tostring(L, 1);
    if(script == NULL) {
        luaL_error(L, "Script name argument not supplied");
    }
    varname = (char *)lua_tostring(L, 2);
    if(varname == NULL) {
        luaL_error(L, "Tag name argument not supplied");
    }
    modestring = (char *)lua_tostring(L, 3);
    if(modestring == NULL) {
        luaL_error(L, "Mode string argument not supplied");
    }
    for(n = 0; modestring[n] != '\0'; n++) {
        if(modestring[n] == 'r' || modestring[n] == 'R') {
            mode |= MODE_READ;
        } else if(modestring[n] == 'w' || modestring[n] == 'W') {
            mode |= MODE_WRITE;
        } else {
            luaL_error(L, "Invalid mode string");
        }
    }
    if(_add_global(script, varname, mode) < 0) {
        luaL_error(L, "Problem adding tag to global registry");
    }
    return 0;
}

/* Adds a static definition to the *globals list.  A Static
 * variable is one that will live between calls to the script
 * but that are not written back to the OpenDAX server.
 * The function takes two arguments, script name, static variable name */
/* TODO: should this be allowed in a normal script?  I guess
 I don't want any limits but couldn't the scriptname be assumed
 if run from a normal script or given if meant for another script? */
static int
_register_static(lua_State *L)
{
    char *script, *varname;
    
    script = (char *)lua_tostring(L, 1);
    if(script == NULL) {
        luaL_error(L, "Script name argument not supplied");
    }
    varname = (char *)lua_tostring(L, 2);
    if(varname == NULL) {
        luaL_error(L, "Variable name argument not supplied");
    }
    if(_add_global(script, varname, MODE_STATIC) < 0) {
        luaL_error(L, "Problem adding variable to global registry");
    }
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
    lua_pushcfunction(L, _dax_tag_add);
    lua_setglobal(L, "dax_tag_add");
    
    lua_pushcfunction(L, _dax_read);
    lua_setglobal(L, "dax_read");
    
    lua_pushcfunction(L, _dax_write);
    lua_setglobal(L, "dax_write");
    
    lua_pushcfunction(L, _register_tag);
    lua_setglobal(L, "register_tag");
    
    lua_pushcfunction(L, _register_static);
    lua_setglobal(L, "register_static");
    
    lua_pushcfunction(L, _cdt_create);
    lua_setglobal(L, "dax_cdt_create");
    
    /* register the libraries that we need*/
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);
    
    return 0; /* I don't think anything returns an error */
}
