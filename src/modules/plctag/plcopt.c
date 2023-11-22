/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2023 Phil Birkelbach
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
 */


#define _GNU_SOURCE
#include "plctag.h"
#include <signal.h>
#include <libdaxlua.h>


extern dax_state *ds;
extern struct tagdef *taglist;

static int
_add_plctag(lua_State *L)
{
    int luatype;
    char *plctag, *daxtag;
    int count, read_update, write_update;

    struct tagdef *newtag;

    dax_log(DAX_LOG_DEBUG, "Adding a plc tag");

    if(!lua_istable(L, -1)) {
        luaL_error(L, "add_plctag() received an argument that is not a table");
    }

    luatype = lua_getfield(L, -1, "daxtag");
    if(luatype == LUA_TNIL) {
        luaL_error(L, "'daxtag' required");
    }
    daxtag = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    luatype = lua_getfield(L, -1, "plctag");
    if(luatype == LUA_TNIL) {
        luaL_error(L, "plctag identifier is required for tag '%s'", daxtag);
    }
    plctag = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    // luatype = lua_getfield(L, -1, "type");
    // if(luatype == LUA_TNIL) {
    //     luaL_error(L, "type is required for tag '%s'", daxtag);
    // } else if(luatype == LUA_TSTRING) {
    //     typestr = (char *)lua_tostring(L, -1);
    // } else {
    //     type = lua_tointeger(L, -1);
    // }
    // lua_pop(L, 1);

    lua_getfield(L, -1, "count");
    count = lua_tointeger(L, -1); /* If missing this should be zero which is no update*/
    if(count == 0) count = 1;
    lua_pop(L, 1);

    lua_getfield(L, -1, "read_update");
    read_update = lua_tointeger(L, -1); /* If missing this should be zero which is no update*/
    lua_pop(L, 1);

    lua_getfield(L, -1, "write_update");
    write_update = lua_tointeger(L, -1);
    lua_pop(L, 1);

    newtag = malloc(sizeof(struct tagdef));
    if(newtag == NULL) {
        luaL_error(L, "Unable to allocate memory for tag");
    }
    bzero(newtag, sizeof(struct tagdef));
    newtag->daxtag = strdup(daxtag);
    newtag->plctag = strdup(plctag);
    // newtag->type = type;
    // if(typestr != NULL) {
    //     newtag->typestr = strdup(typestr);
    // } else {
    //     newtag->typestr = NULL;
    // }
    newtag->count = count;
    newtag->read_update = read_update;
    newtag->write_update = write_update;
    /* Pushing it onto the top is much easier */
    newtag->next = taglist;
    taglist = newtag;

    return 0;
}

static void
_print_config(void) {
    struct tagdef *this;

    this = taglist;
    while(this != NULL) {
        printf("name:%s, tag:%s, read:%d, write:%d\n", this->daxtag, this->plctag, this->read_update, this->write_update);
        this = this->next;
    }
}

static void
_set_constants(lua_State *L) {
    uint32_t _ints[] = {
        DAX_BOOL,
        DAX_BYTE,
        DAX_SINT,
        DAX_CHAR,
        DAX_WORD,
        DAX_INT,
        DAX_UINT,
        DAX_DWORD,
        DAX_DINT,
        DAX_UDINT,
        DAX_REAL,
        DAX_LWORD,
        DAX_LINT,
        DAX_ULINT,
        DAX_TIME,
        DAX_LREAL
    };
    const char *_str[] = {
        "BOOL",
        "BYTE",
        "SINT",
        "CHAR",
        "WORD",
        "INT",
        "UINT",
        "DWORD",
        "DINT",
        "UDINT",
        "REAL",
        "LWORD",
        "LINT",
        "ULINT",
        "TIME",
        "LREAL"
    };
    for(int n=0;n<16;n++) {
        lua_pushinteger(L, _ints[n]);
        lua_setglobal(L, _str[n]);
    }
}

/* This function should be called from main() to configure the program.
 * First the defaults are set then the configuration file is parsed then
 * the command line is handled.  This gives the command line priority.  */
int
plctag_configure(int argc, char *argv[])
{
    int result = 0;
    lua_State *L;

    L = dax_get_luastate(ds);
    _set_constants(L);
    dax_set_luafunction(ds, (void *)_add_plctag, "add_plctag");

    result = dax_configure(ds, argc, (char **)argv, CFG_CMDLINE | CFG_MODCONF);

    //dax_clear_luafunction(ds, "add_plctag");
    _print_config();

    return result;
}

