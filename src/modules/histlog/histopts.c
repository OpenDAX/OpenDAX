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
 *  Configuration options source file for the opendax historical logging
 *  module.
 */

#include <opendax.h>
#include "histlog.h"

extern dax_state *ds;

static int
_add_tag(lua_State *L) {
    int arg_count;
    const char *tagname;
    uint32_t trigger;
    double trigger_value;
    const char *attributes;

    arg_count = lua_gettop(L);
    if(arg_count < 3) {
        luaL_error(L, "add_tag() - Must have at least three arguments");
        return 0;
    }
    tagname = lua_tolstring(L, 1, NULL);
    trigger = lua_tointeger(L, 2);
    trigger_value = lua_tonumber(L, 3);
    if(arg_count >= 4) {
        attributes = lua_tolstring(L, 4, NULL);
    }

    DF("Tagname = %s", tagname);
    DF("Trigger = 0x%X", trigger);
    DF("Value = %f", trigger_value);
    DF("Attributes = %s", attributes);
    return 0;
}


int
histlog_configure(int argc,char *argv[]) {
    int flags;
    int result = 0;
    lua_State *L;

    /* Create and initialize the configuration subsystem in the library */
    dax_init_config(ds, "histlog");

    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "plugin","plugin", 'p', flags, NULL);
    L = dax_get_luastate(ds);

    /* Add globals to the Lua Configuration State. */
    /* Recording Triggers */
    lua_pushinteger(L, ABS_CHANGE);  lua_setglobal(L, "ABS_CHANGE");
    lua_pushinteger(L, PCT_CHANGE);  lua_setglobal(L, "PCT_CHANGE");
    lua_pushinteger(L, ON_WRITE);  lua_setglobal(L, "WRITE");

    dax_set_luafunction(ds, (void *)_add_tag, "add_tag");

    /* Add the functions that will be called */
    /* Execute the configuration */
    dax_configure(ds, argc, argv, CFG_CMDLINE | CFG_MODCONF);

    /* Free the configuration data */
    dax_free_config(ds);
    return 0;
}
