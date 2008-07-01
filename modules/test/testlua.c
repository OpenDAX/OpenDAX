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
            _tests_failed++;
            return -1;
        }
        lua_pop(L, 1);
    }
    
    return 0;
}


/* Adds the functions to the Lua State */
void
add_test_functions(lua_State *L) {
    lua_pushcfunction(L, _add_random_tags);
    lua_setglobal(L, "add_random_tags");

    lua_pushcfunction(L, _check_tagnames);
    lua_setglobal(L, "check_tagnames");

}
