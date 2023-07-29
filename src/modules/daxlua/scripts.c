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
    scripts[n].L = NULL;
    scripts[n].name =  "";
    scripts[n].globals = NULL;
    scripts[n].firstrun = 1;
    scripts[n].name = NULL;
    return &scripts[n];
}

/* This effectively deletes the last script.  It should only be called right
   after the get_new_script() function. */
void
del_script(void) {
   scriptcount--;
}

/* Looks into the list of tags in the script and reads those tags
 * from the server.  Then makes these tags global Lua variables
 * to the script */
static inline int
_receive_globals(script_t *s)
{
    global_t *this;

    this = s->globals;

    while(this != NULL) {
        if(this->mode & MODE_READ) {
            if(fetch_tag(s->L, this->handle)) {
                return -1;
            } else {
                lua_setglobal(s->L, this->name);
            }
        } else if((this->mode & MODE_STATIC) && this->ref != LUA_NOREF) {
            lua_rawgeti(s->L, LUA_REGISTRYINDEX, this->ref);
            lua_setglobal(s->L, this->name);
        }
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

    lua_pushinteger(s->L, (lua_Integer)s->rate);
    lua_setglobal(s->L, "_rate");

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
        if(this->mode & MODE_WRITE) {
            lua_getglobal(s->L, this->name);

            if(send_tag(s->L, this->handle)) {
                return -1;
            }
            lua_pop(s->L, 1);
        } else if(this->mode & MODE_STATIC) {
            lua_getglobal(s->L, this->name);
            if(this->ref == LUA_NOREF) {
                this->ref = luaL_ref(s->L, LUA_REGISTRYINDEX);
            } else {
                lua_rawseti(s->L, LUA_REGISTRYINDEX, this->ref);
            }
        }
        this = this->next;
    }

    lua_getglobal(s->L, "_rate");
    s->rate = lua_tointeger(s->L, -1);
    if(s->rate < 0) s->rate = 1000;
    lua_pop(s->L, 1);

    return 0;
}

/* Converts the lua_Number that we would get off of the Lua stack into
 * the proper form and assigns it to the write member of the union 'dest'
 * a pointer to this union can then be passed to dax_event_add() as the
 * data argument for EQUAL, GREATER, LESS and DEADBAND events. */
static inline void
_convert_lua_number(tag_type datatype, dax_type_union *dest, lua_Number x) {

    switch(datatype) {
        case DAX_BYTE:
            dest->dax_byte = (dax_byte)x;
            return;
        case DAX_SINT:
        case DAX_CHAR:
            dest->dax_sint = (dax_sint)x;
            return;
        case DAX_UINT:
        case DAX_WORD:
            dest->dax_uint = (dax_uint)x;
            return;
        case DAX_INT:
            dest->dax_int = (dax_int)x;
            return;
        case DAX_UDINT:
        case DAX_DWORD:
        case DAX_TIME:
            dest->dax_udint = (dax_udint)x;
            return;
        case DAX_DINT:
            dest->dax_dint = (dax_dint)x;
            return;
        case DAX_ULINT:
        case DAX_LWORD:
            dest->dax_ulint = (dax_ulint)x;
            return;
        case DAX_LINT:
            dest->dax_lint = (dax_lint)x;
            return;
        case DAX_REAL:
            dest->dax_real = (dax_real)x;
            return;
        case DAX_LREAL:
            dest->dax_lreal = (dax_lreal)x;
            return;
        default:
            dest->dax_ulint = 0L;
    }
    return;
}

// static void
// _script_event_dispatch(dax_state *d, void *udata) {
//     script_t *s;
//     s = (script_t *)udata;
//     pthread_mutex_lock(&s->mutex);
//     pthread_cond_signal(&s->condition);
//     pthread_mutex_unlock(&s->mutex);
// }

static void
_script_event_dispatch(dax_state *d, void *udata) {
   DF("Do some stuff");
}

static inline int
_setup_script_event(lua_State *L, script_t *s) {
    tag_handle h;
    dax_type_union u;
    int result;

    result = dax_tag_handle(ds, &h, s->event_tagname, s->event_count);
    if(result) {
        return result;
    }

    _convert_lua_number(h.type, &u, s->event_value);
    result = dax_event_add(ds, &h, s->event_type, (void *)&u,
                           NULL, _script_event_dispatch, s, NULL);
    if(result) {
        return(result);
    }
    return 0;
}

void
run_script(script_t *s) {
    struct timespec start;

    /* In this case do nothing */
    if(s->L == NULL || s->enable == 0) {
        return;
    }
    /* Get the time since system startup (in Linux) */
    clock_gettime(CLOCK_MONOTONIC, &start);
    /* This is the time that we are running the script now in mSec since system startup */
    s->thisscan = start.tv_sec*1000 + start.tv_nsec / 1000;

    /* retrieve the funciton and put it on the stack */
    lua_rawgeti(s->L, LUA_REGISTRYINDEX, s->func_ref);

    /* Get the configured tags and set the globals for the script */
    if(_receive_globals(s)) {
        dax_log(LOG_ERROR, "Unable to find all the global tags\n");
    } else {
        /* Run the script that is on the top of the stack */
        if( lua_pcall(s->L, 0, 0, 0) ) {
            dax_log(LOG_ERROR, "Error Running Script - %s", lua_tostring(s->L, -1));
        }
        /* Write the configured global tags out to the server */
        /* TODO: Should we do something if this fails, if register globals
           works then this should too */
        _send_globals(s);
        s->executions++;
        s->firstrun = 0;
    }

    /* Set the lastscan value in uSec */
    s->lastscan = start.tv_sec*1000 + start.tv_nsec / 1000;

}

/* This is the actual script thread function.  Here we run each periodic
 * script in it's own thread with it's own delay. */
// int
// lua_script_thread(script_t *s)
// {
//     lua_State *L;

//     /* Create a lua interpreter object */
//     L = luaL_newstate();
//     setup_interpreter(L);
//     /* load and compile the file */
//     if(luaL_loadfile(L, s->filename) ) {
//         dax_log(LOG_ERROR, "Error Loading Main Script - %s", lua_tostring(L, -1));
//         return 1;
//     }
//     /* Basicaly stores the Lua script */
//     s->func_ref = luaL_ref(L, LUA_REGISTRYINDEX);

//     if(s->trigger) {
//         pthread_cond_init(&s->condition, NULL);
//         pthread_mutex_init(&s->mutex, NULL);
//         _setup_script_event(L, s);
//     }

//     /* Main Infinite Loops */
//     if(s->trigger) {
//         while(1) { /* This is the event driven loop */
//             pthread_mutex_lock(&s->mutex);
//             pthread_cond_wait(&s->condition, &s->mutex);
//             pthread_mutex_unlock(&s->mutex);
//             if(s->enable) {
//                 _run_script(L, s);
//             }
//         }
//     } else {
//         while(1) { /* This is the periodic loop */
//             if(s->enable) {
//                 _run_script(L, s);
//                 /* If it takes longer than the scanrate then just go again instead of sleeping */
//                 if(s->lastscan < s->rate)
//                     usleep((s->rate - s->lastscan) * 1000);
//             } else {
//                 usleep(s->rate * 1000);
//             }
//         }
//     }
//     /* Clean up and get out */
//     lua_close(L);

//     /* Should never get here */
//     return 1;
// }


/* This function attempts to start the thread given by s */
// static int
// _start_thread(script_t *s)
// {
//     pthread_attr_t attr;

//     pthread_attr_init(&attr);
//     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

//     if(pthread_create(&s->thread, &attr, (void *)&lua_script_thread, (void *)s)) {
//         dax_log(LOG_ERROR, "Unable to start thread for script - %s", s->name);
//         return -1;
//     } else {
//         dax_log(LOG_MAJOR, "Started Thread for script - %s", s->name);
//         return 0;
//     }
// }

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

