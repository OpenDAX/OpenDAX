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

/* This is main source file for the the PLCTAG client module.  This module
   uses the features of libplctag to read/write tags from supported PLCs.
   See the documentation of libplctag at https://github.com/libplctag/libplctag
   for supported PLCs. */

#define _GNU_SOURCE
#include "plctag.h"
#include <signal.h>
#include <libdaxlua.h>

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;
static int _runsignal;
struct tagdef *taglist = NULL;

void
_run_function(dax_state *ds, void *ud) {
    _runsignal = 1;
    dax_log(DAX_LOG_DEBUG, "Run signal received from tagserver");
    dax_default_run(ds, ud);
}

void
_stop_function(dax_state *ds, void *ud) {
    _runsignal = 0;
    dax_log(DAX_LOG_DEBUG, "Stop signal received from tagserver");
    dax_default_stop(ds, ud);
}

void
_kill_function(dax_state *ds, void *ud) {
    _quitsignal = 1;
    dax_log(DAX_LOG_MINOR, "Kill signal received from tagserver");
    dax_default_kill(ds, ud);
}


static int
_add_plctag(lua_State *L)
{
    int luatype;
    char *plctag, *name;
    int type, read_update, write_update;

    struct tagdef *newtag;

    dax_log(DAX_LOG_DEBUG, "Adding a plc tag");

    if(!lua_istable(L, -1)) {
        luaL_error(L, "add_plctag() received an argument that is not a table");
    }

    luatype = lua_getfield(L, -1, "name");
    if(luatype == LUA_TNIL) {
        luaL_error(L, "'name' required");
    }
    name = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    luatype = lua_getfield(L, -1, "plctag");
    if(luatype == LUA_TNIL) {
        luaL_error(L, "plctag identifier is required for tag '%s'", name);
    }
    plctag = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    luatype = lua_getfield(L, -1, "type");
    if(luatype == LUA_TNIL) {
        luaL_error(L, "type is required for tag '%s'", name);
    }
    type = lua_tointeger(L, -1);
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

    newtag->name = strdup(name);
    newtag->plctag = strdup(plctag);
    newtag->type = type;
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
        printf("name:%s, tag:%s, type:%d, read:%d, write:%d\n", this->name, this->plctag, this->type, this->read_update, this->write_update);
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

    dax_clear_luafunction(ds, "add_plctag");
    _print_config();

    return result;
}

/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;
    int result = 0;

    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("plctag"); /* Replace 'skel' with your module name */
    if(ds == NULL) {
        dax_log(DAX_LOG_FATAL, "Unable to Allocate DaxState Object\n");
    }

    /* Execute the configuration */
    result = plctag_configure(argc, argv);

    dax_free_config (ds);

    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_log(DAX_LOG_FATAL, "Unable to find OpenDAX");
        exit(-1);
    }

    _runsignal = 1;
    result = dax_set_run_callback(ds, _run_function);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to add run callback - %s", dax_errstr(result));
    }
    result = dax_set_stop_callback(ds, _stop_function);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to add stop callback - %s", dax_errstr(result));
    }
    result = dax_set_kill_callback(ds, _kill_function);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to add kill callback - %s", dax_errstr(result));
    }
    dax_set_status(ds, "OK");

    dax_log(DAX_LOG_MINOR, "PLCTAG Module Starting");
    dax_set_running(ds, 1);
    while(1) {
    	/* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_log(DAX_LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        dax_event_wait(ds, 1000, NULL);
    }

 /* This is just to make the compiler happy */
    return(0);
}

/* Signal handler */
void
quit_signal(int sig)
{
    _quitsignal = sig;
}

/* We call this function to exit the program */
static void
getout(int exitstatus)
{
    dax_disconnect(ds);
    exit(exitstatus);
}
