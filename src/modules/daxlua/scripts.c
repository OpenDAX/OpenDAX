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

 * Source file for handling the scripts
 */

#include "daxlua.h"
#include <pthread.h>
#include <signal.h>
#include <time.h>


static script_t *scripts = NULL;
static int scripts_size = 0;
static int scriptcount = 0;
extern dax_state *ds;

/* This function returns an index into the scripts[] array for
   the next unassigned script */
script_t *
get_new_script(void)
{
    void *ns;
    int n;

    /* Allocate the script array if it has not already been done */
    if(scriptcount == 0) {
        scripts = malloc(sizeof(script_t) * NUM_SCRIPTS);
        if(scripts == NULL) {
            dax_log(LOG_FATAL, "Cannot allocate memory for the scripts");
            kill(getpid(), SIGQUIT);
        }
        scripts_size = NUM_SCRIPTS;
    } else if(scriptcount == scripts_size) {
        ns = realloc(scripts, sizeof(script_t) * (scripts_size + NUM_SCRIPTS));
        if(ns != NULL) {
            scripts = ns;
        } else {
            dax_log(LOG_ERROR, "Failure to allocate additional scripts");
            return NULL;
        }
    }
    n = scriptcount;
    scriptcount++;
    /* Initialize the script structure */
    scripts[n].L = luaL_newstate();
    scripts[n].name =  "";
    scripts[n].globals = NULL;
    scripts[n].event_buff = NULL;
    scripts[n].firstrun = 1;
    scripts[n].name = NULL;
    scripts[n].failed = 0;
    return &scripts[n];
}

/* This effectively deletes the last script.  It should only be called right
   after the get_new_script() function. */
void
del_script(void) {
   scriptcount--;
}

int
add_global(script_t *script, char *varname, char* tagname, unsigned char mode) {
    global_t *glo;

    glo = malloc(sizeof(global_t));
    if(glo) {
        glo->name = strdup( varname );
        glo->tagname = strdup( tagname );
        if(glo->name == NULL || glo->tagname == NULL) {
            free(glo);
            return ERR_ALLOC;
        }
        glo->mode = mode;
        glo->ref = LUA_NOREF;
        glo->next = script->globals;
        script->globals = glo;
    } else {
        return ERR_ALLOC;
    }
    return 0;
}

/* This function creates a static variable from the value that is on
   top of the Lua stack that is passed. */
int
add_static(script_t *script, lua_State *L, char* varname) {
    global_t *glo;
    int lt;

    glo = malloc(sizeof(global_t));
    if(glo) {
        glo->name = strdup( varname );
        glo->mode = MODE_STATIC | MODE_INIT;

        /* since we cannot simply copy a value from one stack to another (lua_xmove does not
           work since these are not in the same thread) we are limiting our values to numbers
           strings and booleans. To move tables we'd have to manually traverse the table and
           it's just not worth it. */
        lt = lua_type(L, -1);
        if(lt == LUA_TBOOLEAN) {
            lua_pushboolean(script->L, lua_toboolean(L, -1));
        } else if(lt == LUA_TNUMBER) {
            if(lua_isinteger(L, -1)) {
                lua_pushinteger(script->L, lua_tointeger(L, -1));
            } else {
                lua_pushnumber(script->L, lua_tonumber(L, -1));
            }
        } else if(lt == LUA_TSTRING) {
            lua_pushstring(script->L, lua_tostring(L, -1));
        } else {
            dax_log(LOG_WARN, "Static variable, '%s', cannot be %s", varname, luaL_typename(L, -1));
            lua_pushnil(script->L);
        }
        glo->ref = luaL_ref(script->L, LUA_REGISTRYINDEX);

        glo->next = script->globals;
        script->globals = glo;
    } else {
        return ERR_ALLOC;
    }
    return 0;
}



/* Looks into the list of tags in the script and reads those tags
 * from the server.  Then makes these tags global Lua variables
 * to the script */
static inline int
_receive_globals(script_t *s) {
    int result;
    global_t *this;

    this = s->globals;

    while(this != NULL) {
        /* If we are not initialized yet then we should try to get a handle */
        if(! (this->mode & MODE_INIT) ) {
            result = dax_tag_handle(ds, &this->handle, this->tagname, 0);
            if(result == 0) {
                /* If successfull then set the INIT flag */
                this->mode = this->mode | MODE_INIT;
            } else {
                dax_log(LOG_DEBUG, "Failed to get tag %s.  Will retry.", this->tagname);
            }
        }
        if(this->mode & MODE_READ && this->mode & MODE_INIT) {
            if(fetch_tag(s->L, this->handle)) {
                // TODO we might should unitialize this tag if it has been deleted
                lua_pushnil(s->L);
            }
        } else if((this->mode & MODE_STATIC) && this->ref != LUA_NOREF) {
            lua_rawgeti(s->L, LUA_REGISTRYINDEX, this->ref);
        } else {
            lua_pushnil(s->L);
        }
        lua_setglobal(s->L, this->name);
        this = this->next;
    }

    /* Now we set the daxlua system values as global variables */
    lua_pushstring(s->L, s->name);
    lua_setglobal(s->L, "_name");

    /* This may be of liimited usefulness */
    lua_pushstring(s->L, s->filename);
    lua_setglobal(s->L, "_filename");

    lua_pushinteger(s->L, (lua_Integer)s->lastscan);
    lua_setglobal(s->L, "_lastscan");

    lua_pushinteger(s->L, (lua_Integer)s->thisscan);
    lua_setglobal(s->L, "_thisscan");

    lua_pushinteger(s->L, (lua_Integer)s->interval);
    lua_setglobal(s->L, "_interval");

    lua_pushinteger(s->L, (lua_Integer)s->executions);
    lua_setglobal(s->L, "_executions");

    lua_pushboolean(s->L, s->firstrun);
    lua_setglobal(s->L, "_firstrun");

    return 0;
}

/* Looks into the list of tags in the script and reads these global
   variables from the script and then writes the values out to the
   server */
static inline int
_send_globals(script_t *s)
{
    global_t *this;

    this = s->globals;

    while(this != NULL) {
        /* If the write mode flag is set and we are initialized */
        if(this->mode & MODE_WRITE && this->mode & MODE_INIT) {
            lua_getglobal(s->L, this->name);

            if(send_tag(s->L, this->handle)) {
                return -1;
            }
            lua_pop(s->L, 1);
        }

        this = this->next;
    }

    return 0;
}

void
run_script(script_t *s) {
    struct timespec start;

    /* In this case do nothing */
    if(s->failed || s->enable == 0) {
        return;
    }

    /* Set the lastscan value in uSec */
    s->lastscan = s->thisscan;
    /* Get the time since system startup (in Linux) */
    clock_gettime(CLOCK_MONOTONIC, &start);
    /* This is the time that we are running the script now in mSec since system startup */
    s->thisscan = start.tv_sec*1e6 + start.tv_nsec / 1000;

    /* retrieve the funciton and put it on the stack */
    lua_rawgeti(s->L, LUA_REGISTRYINDEX, s->func_ref);

    /* Get the configured tags and set the globals for the script */
    if(_receive_globals(s)) {
        dax_log(LOG_ERROR, "Unable to find all the global tags\n");
    } else {
        /* Run the script that is on the top of the stack */
        if( lua_pcall(s->L, 0, 0, 0) ) {
            dax_log(LOG_ERROR, "Error Running Script %s - %s",s->name, lua_tostring(s->L, -1));
            s->failed = 1;
        }
        /* Write the configured global tags out to the server */
        /* TODO: Should we do something if this fails, if register globals
           works then this should too */
        _send_globals(s);
        s->executions++;
        s->firstrun = 0;
    }

}

int
start_all_scripts(void)
{
    int n;

    thread_start_all();

    if(scriptcount) {
        for(n = 0; n < scriptcount; n++) {
            if(scripts[n].L == NULL) {
                dax_log(LOG_WARN, "Script '%s' is not assigned to a thread so it will never run!", scripts[n].name);
            }

        }
    }

    return 0;
}

int
get_script_count(void) {
    return scriptcount;
}

script_t *
get_script(int index) {
    return &scripts[index];
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

