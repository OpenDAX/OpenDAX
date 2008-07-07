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

#include <daxlua.h>
#include <string.h>
#include <getopt.h>
#include <math.h>


/* TODO: Allow user to set whether an error in a script is fatal */

static char *initscript;
static script_t *scripts = NULL;
static int scripts_size = 0;
static int scriptcount = 0;

/* This function returns an index into the scripts[] array for
   the next unassigned script */
static int
_get_new_script(void)
{
    void *ns;
    
    /* Allocate the script array if it has not already been done */
    if(scriptcount == 0) {
        scripts = malloc(sizeof(script_t) * NUM_SCRIPTS);
        if(scripts == NULL) {
            dax_fatal("Cannot allocate memory for the scripts");
        }
        scripts_size = NUM_SCRIPTS;
    } else if(scriptcount == scripts_size) {
        ns = realloc(scripts, sizeof(script_t) * (scripts_size + NUM_SCRIPTS));
        if(ns != NULL) {
            scripts = ns;
        } else {
            dax_error("Failure to allocate additional scripts");
            return -1;
        }
    }
    scriptcount++;
    return scriptcount - 1;
}

/* Lua interface function for adding a modbus command to a port.  
 Accepts two arguments.  The first is the port to assign the command
 too, and the second is a table describing the command. */
static int
_add_script(lua_State *L)
{
    int si;
    char *string;
    
    if(! lua_istable(L, 1) ) {
        luaL_error(L, "Table needed to add script");
    }
    
    si = _get_new_script();
    
    if(si < 0) {
        /* Just bail for now */
        return 0;
    }
    /* Initialize the script structure */
    scripts[si].globals = NULL;
    scripts[si].firstrun = 1;
    
    lua_getfield(L, 1, "enable");
    if( lua_isnil(L, -1)) {
        scripts[si].enable = 1;
    } else {
        scripts[si].enable = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "name");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        /* check for duplicate name */
        if(get_script_name(string)) {
            scriptcount--; /* This effectively deletes the script */
            luaL_error(L, "duplicate script name %s", string);
        }
        scripts[si].name = strdup(string);
    } else {
        luaL_error(L, "name is required for a script");
        scripts[si].name = NULL;
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "filename");
    string = (char *)lua_tostring(L, -1);
    if(string) {
        scripts[si].filename = strdup(string);
    } else {
        scripts[si].filename = NULL;
        scripts[si].enable = 0;
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "rate");
    scripts[si].rate = lua_tointeger(L, -1);
    if(scripts[si].rate <= 0) {
        scripts[si].rate = DEFAULT_RATE;
    }
    lua_pop(L, 1);
    
    return 0;
}

/* Public function to initialize the module */
int
configure(int argc, char *argv[])
{
    int flags, result = 0;
    
    dax_init_config("daxlua");
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute("initscript", "initscript", 'i', flags, "init.lua");
    if(result) {
        dax_fatal("Problem with the configuration");
    }
    
    dax_set_luafunction((void *)_add_script, "add_script");
    
    dax_configure(argc, (char **)argv, CFG_CMDLINE | CFG_DAXCONF | CFG_MODCONF);

    initscript = strdup(dax_get_attr("initscript"));
    
    dax_free_config();
    
    return 0;
}

/* Configuration retrieval functions */
char *
get_init(void)
{
    return initscript;
}

int
get_scriptcount(void)
{
    return scriptcount;
}

script_t *
get_script(int index)
{
    if(index < 0 || index >= scriptcount) {
        return NULL;
    } else {
        return &scripts[index];
    }
}

script_t *
get_script_name(char *name)
{
    int n;
    
    for(n = 0; n < scriptcount; n++) {
        if( scripts[n].name && name && ! strcmp(scripts[n].name, name)) {
            return &scripts[n];
        }
    }
    return NULL;
}
