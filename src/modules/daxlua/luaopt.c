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

 * Configuration source code file for the Lua script interpreter
 */

#include "daxlua.h"
#include <getopt.h>
#include <math.h>
#include <signal.h>

/* TODO: Allow user to set whether an error in a script is fatal */

extern dax_state *ds;


/* When this function is called it is expected that there
 * is a table on the top of the Lua stack that represents
 * the trigger */
static int
_set_trigger(lua_State *L, script_t *s) {
    char *string;

    lua_getfield(L, -1, "tag");
    s->event_tagname = strdup((char *)lua_tostring(L, -1));
    if(s->event_tagname == NULL) {
        luaL_error(L, "'tagname' is required for an event trigger");
        s->trigger = 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "type");
    string = (char *)lua_tostring(L, -1);
    s->event_type = dax_event_string_to_type(string);
    if(s->event_type == 0) {
        luaL_error(L, "'type' event type is required for an event trigger");
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "count");
    s->event_count = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "value");
    s->event_value = lua_tonumber(L, -1);
    lua_pop(L, 1);

    s->trigger = 1;
    return 0;
}

static int
_add_script(lua_State *L)
{
    script_t *si;
    char *string;

    if(! lua_istable(L, 1) ) {
        luaL_error(L, "Table needed to add script");
    }

    si = get_new_script();

    if(si == NULL) {
        /* Just bail for now */
        return 0;
    }

    lua_getfield(L, 1, "enable");
    if(lua_isnil(L, -1)) {
        si->enable = 1;
    } else {
        si->enable = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "thread");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        si->threadname = strdup(string);
    } else {
        si->threadname = NULL;
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "trigger");
    if(lua_istable(L, -1)) {
        _set_trigger(L, si);
    } else {
        si->trigger = 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "name");
    string = (char *)lua_tostring(L, -1);
    DF("We got a script name of %s", string);
    if(string) {
        /* check for duplicate name */
        if(get_script_name(string)) {
            del_script(); /* This effectively deletes the script */
            luaL_error(L, "duplicate script name %s", string);
        }
        si->name = strdup(string);
    } else {
        luaL_error(L, "name is required for a script");
        si->name = NULL;
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "filename");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        si->filename = strdup(string);
    } else {
        si->filename = NULL;
        si->enable = 0;
    }
    lua_pop(L, 1);

    return 0;
}

static int
_add_interval_thread(lua_State *L) {
    char *name;
    long interval;

    if(lua_gettop(L) != 2) {
        luaL_error(L, "add_interval_thread function requires two arguments (name, interval)");
    }
    if(lua_isstring(L, 1)) {
        name = (char *)lua_tostring(L, 1);
    } else {
        luaL_error(L, "first argument to add_interval_thread function must be a string");
    }
    if(lua_isnumber(L, 2)) {
        interval = lua_tonumber(L, 2);
    } else {
        luaL_error(L, "second argument to add_interval_thread must be a number");
    }

    add_interval_thread(name, interval);
    return 0;
}


/* Public function to initialize the module */
int
configure(int argc, char *argv[])
{
    int flags, result = 0;
    char *s;

    dax_init_config(ds, "daxlua");
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "event_thread_count", "event_thread_count", 'n', flags, "8");
    if(result) {
         dax_log(LOG_FATAL, "Problem with the configuration");
    }
    result += dax_add_attribute(ds, "event_queue_size", "event_queue_size", 's', flags, "128");
    if(result) {
         dax_log(LOG_FATAL, "Problem with the configuration");
    }

    dax_set_luafunction(ds, (void *)_add_script, "add_script");
    dax_set_luafunction(ds, (void *)_add_interval_thread, "add_interval_thread");

    result = dax_configure(ds, argc, (char **)argv, CFG_CMDLINE | CFG_MODCONF);

    s = dax_get_attr(ds, "event_thread_count");
    if(s != NULL)
        thread_set_qthreadcount(strtoul(s, NULL, 10));
    s = dax_get_attr(ds, "event_queue_size");
    if(s != NULL)
        thread_set_queue_size(strtoul(s, NULL, 10));

    dax_free_config(ds);

    return result;
}

