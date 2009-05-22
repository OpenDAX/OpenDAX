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
 *  This file contains the wrapper functions and lua definitions for the
 *  opendax daxtest module.
 */

#include <daxtest.h>

/* These are to keep score */
static int _tests_run = 0;
static int _tests_failed = 0;

/* Functions to keep the above from having to be extern global */
int
tests_run(void)
{
    return _tests_run;
}

int
tests_failed(void)
{
    return _tests_failed;
}

/* Adds a certain number of randomly generated tags that start
 * with the given name.  
 * Lua Call : add_random_tags(int count, string name) */
static int
_add_random_tags(lua_State *L)
{
    int count;
    char *name;
    
    if(lua_gettop(L) != 2) {
        luaL_error(L, "wrong number of arguments to add_random_tags()");
    }
    
    if(! lua_isnumber(L, 1)) {
        luaL_error(L, "argument to add_random_tags() is not a number");
    }
    count = lua_tointeger(L, 1);
    name = (char *)lua_tostring(L, 2);
    _tests_run++;
    if(add_random_tags(count, name)) {
        dax_error("Failed add_random_tags(%d, %s)\n", count, name);
        _tests_failed++;
    }
    return 0;
}

/* Checks that tagnames that should fail do so and that
 * tagnames that are supposed to pass are added.
 * Lua Call : check_tagnames(string[] tags_to_fail, string[] tags_to_pass) 
 * the string[]'s should be integer indexed arrays. */
static int
_check_tagnames(lua_State *L)
{
    const char *s;
    int n = 1;
    
    if(lua_gettop(L) != 2) {
        luaL_error(L, "wrong number of arguments to check_tagnames()");
    }
    if(! lua_istable(L, 1) || ! lua_istable(L, 2)) {
        luaL_error(L, "both arguments to check_tagnames() should be tables");
    }
    _tests_run++;
    while(1) {
        lua_rawgeti(L, 1, n++);
        if((s = lua_tostring(L, -1)) == NULL) break;
        if(tagtofail(s)) {
            lua_pop(L, 1);
            _tests_failed++;
            return -1;
        }
        lua_pop(L, 1);
    }
    
    n = 1;
    while(1) {
        lua_rawgeti(L, 2, n++);
        if((s = lua_tostring(L, -1)) == NULL) break;
        if(tagtopass(s)) {
            lua_pop(L, 1);
            _tests_failed++;
            return -1;
        }
        lua_pop(L, 1);
    }
    
    return 0;
}

/* Cheesy wrapper for the dax_cdt_create() library function */
/* Right now this isn't a test but it probably should be.  There
 * is no reason we can't set up the environment and test at the 
 * same time. */
static int
_cdt_create(lua_State *L)
{
    int result;
    
    if(lua_gettop(L) != 1) {
        luaL_error(L, "wrong number of arguments to cdt_create()");
    }
    result = dax_cdt_create((char *)lua_tostring(L, -1), NULL);

    if(result == 0) {
        luaL_error(L, "Unable to create datatype %s", lua_tostring(L, -1));
        return -1;
    } else {
        lua_pushinteger(L, result);
    }
    return 1;
}

/* Cheesy wrapper for the dax_cdt_add() library function */
static int
_cdt_add(lua_State *L)
{
    int result;
    type_t mtype, ttype;
    
    if(lua_gettop(L) != 4) {
        luaL_error(L, "wrong number of arguments to cdt_add()");
    }
    ttype = lua_tointeger(L, 1);

    if(lua_isnumber(L, 3)) {
        mtype = lua_tointeger(L, 3);
    } else {
        mtype = dax_string_to_type((char *)lua_tostring(L, 3));
        if(mtype == 0) {
            luaL_error(L, "Whoa, can't get type '%s'", (char *)lua_tostring(L, 3));
        }
    }
    
    result = dax_cdt_add(ttype, (char *)lua_tostring(L, 2), mtype, lua_tointeger(L,4));
    if(result) {
        luaL_error(L, "Unable to Add Member %s", lua_tostring(L, 2));
        return -1;
    }
    return 0;
}

/* Cheesy wrapper for the dax_cdt_finalize() library function */
static int
_cdt_finalize(lua_State *L)
{
    int result;
    type_t type;
    
    if(lua_gettop(L) != 1) {
        luaL_error(L, "wrong number of arguments to check_tagnames()");
    }
    type = lua_tointeger(L, 1);
    result = dax_cdt_finalize(type);
    if(result) {
        luaL_error(L, "Ouch can't finalize 0x%X", type);
    }
    return 0;
}

/* Cheesy wrapper for the dax_tag_add() library function */
static int
_tag_add(lua_State *L)
{
    int result;
    type_t type;
    
    if(lua_gettop(L) != 3) {
        luaL_error(L, "wrong number of arguments to tag_add()");
    }
    if(lua_isnumber(L, 2)) {
        type = lua_tointeger(L, 2);
    } else {
        type = dax_string_to_type((char *)lua_tostring(L, 2));
        if(type == 0) {
            luaL_error(L, "Whoa, can't get type '%s'", (char *)lua_tostring(L, 2));      
        }
    }

    result = dax_tag_add((char *)lua_tostring(L,1), type, lua_tointeger(L,3));
    return 0;
}

/* Lua wrapper for cdt_recursion */
static int
_cdt_recursion(lua_State *L)
{
    _tests_run++;
    if(cdt_recursion()) {
        _tests_failed++;
        return -1;
    }
    return 0;
}

static int
_tag_handle_test(lua_State *L)
{
    handle_t h;
    int n = 1, final = 0;
    const char *name, *type;
    int count, result, byte, bit, rcount, size, test;
    
    if(lua_gettop(L) != 1) {
        luaL_error(L, "wrong number of arguments to tag_handle_test()");
    }
    if(! lua_istable(L, 1) ) {
        luaL_error(L, "should pass a table to tag_handle_test()");
    }
    printf("Starting Tag Handle Test\n");
    _tests_run++;
    while(1) {
        lua_rawgeti(L, 1, n++);
        if(lua_isnil(L, -1)) break;
        lua_rawgeti(L, 2, 1);
        name = lua_tostring(L, -1);
        lua_rawgeti(L, 2, 2);
        count = lua_tointeger(L, -1);
        lua_rawgeti(L, 2, 3);
        byte = lua_tointeger(L, -1);
        lua_rawgeti(L, 2, 4);
        bit = lua_tointeger(L, -1);
        lua_rawgeti(L, 2, 5);
        rcount = lua_tointeger(L, -1);
        lua_rawgeti(L, 2, 6);
        size = lua_tointeger(L, -1);
        lua_rawgeti(L, 2, 7);
        type = lua_tostring(L, -1);
        lua_rawgeti(L, 2, 8);
        test = lua_tointeger(L, -1);
        
        result = dax_tag_handle(&h, (char *)name, count);
        if(test == 1 && result == 0) { /* We should fail */
            printf("Handle Test: %s : %d Should have Failed\n", name, count);
            final = 1;
        } else if(test == 0) {
            if(result) {
                printf("Handle Test: dax_tag_handle failed with code %d\n", result);
                final = 1;
            } else { 
                if(h.byte != byte) {
                    printf("Handle Test: %s : %d - Byte offset does not match (%d != %d)\n", name, count, h.byte, byte);
                    final = 1;
                }
                if(h.bit != bit) {
                    printf("Handle Test: %s : %d - Bit offset does not match (%d != %d)\n", name, count, h.bit, bit);
                    final = 1;
                }
                if(h.count != rcount) {
                    printf("Handle Test: %s : %d - Item count does not match (%d != %d)\n", name, count, h.count, rcount);
                    final = 1;
                }
                if(h.size != size) {
                    printf("Handle Test: %s : %d - Size does not match (%d != %d)\n", name, count, h.size, size);
                    final = 1;
                }
                if(h.type != dax_string_to_type((char *)type)) {
                    printf("Handle Test: %s : %d - Datatype does not match\n", name, count);
                    final = 1;
                }
            }
        }
        lua_pop(L,9);        
    }
    if(final) _tests_failed++;
    return final;
}

/* Adds the functions to the Lua State */
void
add_test_functions(lua_State *L)
{
    lua_pushcfunction(L, _cdt_create);
    lua_setglobal(L, "cdt_create");

    lua_pushcfunction(L, _cdt_add);
    lua_setglobal(L, "cdt_add");

    lua_pushcfunction(L, _cdt_finalize);
    lua_setglobal(L, "cdt_finalize");

    lua_pushcfunction(L, _tag_add);
    lua_setglobal(L, "tag_add");

    lua_pushcfunction(L, _add_random_tags);
    lua_setglobal(L, "add_random_tags");

    lua_pushcfunction(L, _check_tagnames);
    lua_setglobal(L, "check_tagnames");

    lua_pushcfunction(L, _cdt_recursion);
    lua_setglobal(L, "cdt_recursion");
    
    lua_pushcfunction(L, _tag_handle_test);
    lua_setglobal(L, "tag_handle_test");
    
    
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);
}
