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

/* This function runs a test script.  If the script raises an 
 * error this function increments the fail count.  It returns
 * nothing */
static int
_run_test(lua_State *L)
{
    char *script, *desc;
    
    if(lua_gettop(L) != 2) {
        luaL_error(L, "Wrong number of arguments to run_test()");
    }
    
    script = (char *)lua_tostring(L, -2);
    desc = (char *)lua_tostring(L, -1);
    test_start();
    printf("Starting - %s\n", desc);
        
    /* load and run the test script */
    if(luaL_loadfile(L, script)) {
        /* Here the error is allowed to propagate up and kill the whole thing */
        dax_error("Problem loading script - %s", lua_tostring(L, -1));
    }
    if(lua_pcall(L, 0, 0, 0)) {
        test_fail();
        printf("FAILED - %s\n", desc);
    } else {
        printf("PASSED - %s\n", desc);
    }
    return 0;
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
    if(add_random_tags(count, name)) {
        luaL_error(L, "Failed add_random_tags(%d, %s)\n", count, name);
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
    int count, n = 1, result;
    dax_cdt *cdt;
    tag_type type;
    char *name, *typename, *cdt_name;
    
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
        typename = (char *)lua_tostring(L, -1);
        lua_rawgeti(L, 3, 3);
        count = lua_tointeger(L, -1);
        lua_pop(L, 4);
        
        result = dax_cdt_member(cdt, name, dax_string_to_type(typename), count);
        if(result) {
            dax_cdt_free(cdt);
            luaL_error(L, "Unable to add member %s", name);
        }
    }
    result = dax_cdt_create(cdt, &type);
    if(result) {
        luaL_error(L, "Unable to create datatype %s", cdt_name);
    }
    
    lua_pushinteger(L, type);
    return 1;
}


/* Cheesy wrapper for the dax_tag_add() library function */
static int
_tag_add(lua_State *L)
{
    int result;
    tag_type type;
    
    if(lua_gettop(L) != 3) {
        luaL_error(L, "wrong number of arguments to tag_add()");
    }
    if(lua_isnumber(L, 2)) {
        type = lua_tointeger(L, 2);
    } else {
        type = dax_string_to_type((char *)lua_tostring(L, 2));
        if(type == 0) {
            luaL_error(L, "Can't get type '%s'", (char *)lua_tostring(L, 2));      
        }
    }

    result = dax_tag_add(NULL, (char *)lua_tostring(L,1), type, lua_tointeger(L, 3));
    if(result) luaL_error(L, "Unable to add tag '%s'", (char *)lua_tostring(L,1));
    return 0;
}

/* Wrapper for the two tag retrieving functions */
static int
_tag_get(lua_State *L)
{
    int result;
    dax_tag tag;

    if(lua_isnumber(L, 1)) {
        printf("Calling dax_tag_byindex(%d)\n", (tag_index)lua_tointeger(L, 1));
        result = dax_tag_byindex(&tag, (tag_index)lua_tointeger(L, 1));
    } else {
        printf("Calling dax_tag_byname(%s)\n", (char *)lua_tostring(L, 1));
        result = dax_tag_byname(&tag, (char *)lua_tostring(L, 1));
    }
    if(result != 0) {
        printf("result = %d\n", result);
        luaL_error(L, "Can't get tag '%s'", (char *)lua_tostring(L, 2));      
    }
    lua_pushstring(L, tag.name);
    lua_pushstring(L, dax_type_to_string(tag.type));
    lua_pushinteger(L, tag.count);
    return 3;
}

static int
_handle_test(lua_State *L)
{
    Handle h;
    int final = 0;
    const char *name, *type;
    int count, result, byte, bit, rcount, size, test;
    
    if(lua_gettop(L) != 8) {
        luaL_error(L, "wrong number of arguments to tag_handle_test()");
    }
    name = lua_tostring(L, 1);
    count = lua_tointeger(L, 2);
    byte = lua_tointeger(L, 3);
    bit = lua_tointeger(L, 4);
    rcount = lua_tointeger(L, 5);
    size = lua_tointeger(L, 6);
    type = lua_tostring(L, 7);
    test = lua_tointeger(L, 8);
    
    result = dax_tag_handle(&h, (char *)name, count);
    if(test == FAIL && result == 0) { /* We should fail */
        printf("Handle Test: %s : %d Should have Failed\n", name, count);
        final = 1;
    } else if(test == PASS) {
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
    lua_pop(L,8);        

    if(final) {
        luaL_error(L, "Handle Test Failed");
    }
    return 0;
}

/*** LAZY PROGRAMMER TESTS ***********************************
 * This is a temporary place for development of tests.  It puts
 * these tests within the normal testing framework but allows
 * a developer a way to test new library or server features without
 * having to create a formal test.
 *************************************************************/

int static
tag_read_write_test(void)
{
    tag_index index;
    int result;
    Handle handle;
    tag_type type;
    dax_cdt *cdt;
    dax_int write_data[32], read_data[32];
    
    index = dax_tag_add(&handle, "TestReadTagInt", DAX_INT, 32);
    
    result = dax_tag_handle(&handle, "TestReadTagInt", 32);
    assert(result == 0);
    
    write_data[0] = 0x4321;
    dax_write_tag(handle, write_data);
    dax_read_tag(handle, read_data);
    
    printf("read_data[0] = 0x%X\n", read_data[0]);
    
    cdt = dax_cdt_new("TestCDT", NULL);
    dax_cdt_member(cdt, "TheDint", DAX_DINT, 1);
    dax_cdt_member(cdt, "TheInt", DAX_INT, 2);
    result = dax_cdt_create(cdt, &type);
    if(result) {
        printf("Unable to Create TestCDT\n");
        return result;
    }
    
    dax_tag_add(&handle, "TestCDTRead", type, 1);
    dax_tag_handle(&handle, "TestCDTRead.TheInt[1]", 1);
    write_data[1] = 444;
    dax_write_tag(handle, &write_data[1]);
    dax_read_tag(handle, &read_data[1]);
    printf("read_data[1] = %d\n", read_data[1]);
       
    return 0;
}


static int
_lazy_test(lua_State *L)
{
    int result;
    
    /* TODO: Check the corner conditions of the tag reading and writing.
     * Offset, size vs. tag size etc. */
    result = tag_read_write_test();
    if(result) {
        luaL_error(L, "Tag Read/Write Test returned %d", result);
    }
    return 0;
}

/**END OF LAZY PROGRAMMER TESTS**************************************/

/* Adds the functions to the Lua State */
void
add_test_functions(lua_State *L)
{
    lua_pushcfunction(L, _cdt_create);
    lua_setglobal(L, "cdt_create");

    lua_pushcfunction(L, _run_test);
    lua_setglobal(L, "run_test");
    
    lua_pushcfunction(L, _tag_add);
    lua_setglobal(L, "tag_add");

    lua_pushcfunction(L, _tag_get);
    lua_setglobal(L, "tag_get");

    lua_pushcfunction(L, _add_random_tags);
    lua_setglobal(L, "add_random_tags");

    lua_pushcfunction(L, _handle_test);
    lua_setglobal(L, "handle_test");
    
    lua_pushcfunction(L, _lazy_test);
    lua_setglobal(L, "lazy_test");
    
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);
}
