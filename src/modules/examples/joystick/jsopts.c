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
 *  This is the main source file for the OpenDAX joystick module
 **/

#include "daxjoystick.h"

extern dax_state *ds;
extern axis_t axis_list[256];
extern button_t button_list[256];


static int
_set_axis(lua_State *L)
{
    lua_Integer index, offset;
    lua_Number scale;
    const char *tagname;
    int result;

    index = lua_tointegerx(L, 1, &result);
    if(!result || index < 0 || index >255) {
        luaL_error(L, "set_axis() - index argument should be a number between 0-255");
    }
    tagname = lua_tostring(L, 2);
    if(tagname == NULL) {
        luaL_error(L, "set_axis() - problem with tagname argument");
    }
    offset = lua_tointegerx(L, 3, &result);
    if(!result) {
        luaL_error(L, "set_axis() - missing or bad offset argument");
    }
    scale = lua_tonumberx(L, 4, &result);
    if(!result) {
        luaL_error(L, "set_axis() - missing or bad scale argument");
    }
    dax_log(DAX_LOG_DEBUG, "axis[%lld] tagname = %s, offset = %lld, scale = %f", index, tagname, offset, scale);
    axis_list[index].tagname = malloc(strlen(tagname) + 1);
    if(axis_list[index].tagname == NULL) {
        luaL_error(L, "set_axis() - Unable to allocate memory for tagname");
    }
    strcpy(axis_list[index].tagname, tagname);
    axis_list[index].offset = offset;
    axis_list[index].scale = scale;
    return 0;
}

static int
_set_button(lua_State *L)
{
    lua_Integer index;
    const char *tagname;
    int result;

    index = lua_tointegerx(L, 1, &result);
    if(!result || index < 0 || index >255) {
        luaL_error(L, "set_button() index argument should be a number between 0-255");
    }
    tagname = lua_tostring(L, 2);
    if(tagname == NULL) {
        luaL_error(L, "set_button() - problem with tagname argument");
    }
    dax_log(DAX_LOG_DEBUG, "button[%lld] tagname = %s", index, tagname);
    button_list[index].tagname = malloc(strlen(tagname) + 1);
    if(button_list[index].tagname == NULL) {
        luaL_error(L, "set_button() - Unable to allocate memory for tagname");
    }
    strcpy(button_list[index].tagname, tagname);
    return 0;
}


int
configure(int argc, char *argv[]) {
    int flags, result=0;

    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "device","device", 'd', flags, "/dev/input/js0");
/* We might use this to setup some default tags for the axis' and buttons if no config file is present
 * for now it's ignored*/
    result += dax_add_attribute(ds, "prefix","prefix", 'p', flags, "js0_");

    dax_set_luafunction(ds, (void *)_set_axis, "set_axis");
    dax_set_luafunction(ds, (void *)_set_button, "set_button");

    result = dax_configure(ds, argc, argv, CFG_CMDLINE | CFG_MODCONF);

    /* Free the configuration data */
    dax_free_config (ds);
    return result;
}
