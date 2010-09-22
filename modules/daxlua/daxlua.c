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

 * Main source code file for the Lua script interpreter
 */

#include <daxlua.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

static int quitsig = 0;
/* For now we'll keep ds as a global to simplify the code.  At some
 * point when the luaif library is complete it'll be stored in the
 * Lua_State Registry. */
dax_state *ds;
extern pthread_mutex_t daxmutex;

void quit_signal(int sig);

/* Sets up and runs the init script.  The init script runs once
   at module start */
int
lua_init(void)
{
    lua_State *L;
    
    dax_debug(ds, LOG_MAJOR, "Starting init script - %s", get_init());
    /* Create a lua interpreter object */
    L = luaL_newstate();
    setup_interpreter(L);    
    /* load and compile the file */
    if(luaL_loadfile(L, get_init()) || lua_pcall(L, 0, 0, 0) ) {
        dax_error(ds, "Error Running Init Script - %s", lua_tostring(L, -1));
        return 1;
    }
    
    /* Clean up and get out */
    lua_close(L);

    return 0;
}

/* Looks into the list of tags in the script and reads those tags
 * from the server.  Then makes these tags global Lua variables
 * to the script */
static inline int
_receive_globals(lua_State *L, script_t *s)
{
    global_t *this;
    
    this = s->globals;
    
    pthread_mutex_lock(&daxmutex);
    while(this != NULL) {
        if(this->mode & MODE_READ) {
            if(fetch_tag(L, this->handle)) {
                pthread_mutex_unlock(&daxmutex);
                return -1;
            } else {
                lua_setglobal(L, this->name);
            }
        } else if(this->mode & MODE_STATIC && this->ref != LUA_NOREF) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, this->ref);
            lua_setglobal(L, this->name);
        }
        this = this->next;
    }
    
    pthread_mutex_unlock(&daxmutex);
    
    /* Now we set the daxlua system values as global variables */
    lua_pushstring(L, s->name);
    lua_setglobal(L, "_name");
    
    /* This may be of liimited usefulness */
    lua_pushstring(L, s->filename);
    lua_setglobal(L, "_filename");
    
    lua_pushinteger(L, (lua_Integer)s->lastscan);
    lua_setglobal(L, "_lastscan");
    
    lua_pushinteger(L, (lua_Integer)s->rate);
    lua_setglobal(L, "_rate");
    
    lua_pushinteger(L, (lua_Integer)s->executions);
    lua_setglobal(L, "_executions");

    lua_pushboolean(L, s->firstrun);
    lua_setglobal(L, "_firstrun");
    
    return 0;    
}

/* Looks into the list of tags in the script and reads these global
   variables from the script and then writes the values out to the
   server */
/* TODO: It would save some bandwidth if we only wrote tags that have
   changed since we called register_globals() <- configuration?? */
static inline int
_send_globals(lua_State*L, script_t *s)
{
    global_t *this;
    
    this = s->globals;
    pthread_mutex_lock(&daxmutex);
    
    while(this != NULL) {
        if(this->mode & MODE_WRITE) {
            lua_getglobal(L, this->name);
            
            if(send_tag(L, this->handle)) {
                pthread_mutex_unlock(&daxmutex);
                return -1;
            }
            lua_pop(L, 1);
        } else if(this->mode & MODE_STATIC) {
            lua_getglobal(L, this->name);
            if(this->ref == LUA_NOREF) {
                this->ref = luaL_ref(L, LUA_REGISTRYINDEX);
            } else {
                lua_rawseti(L, LUA_REGISTRYINDEX, this->ref);
            }
        }
        this = this->next;
    }
    
    pthread_mutex_unlock(&daxmutex);
    
    lua_getglobal(L, "_rate");
    s->rate = lua_tointeger(L, -1);
    if(s->rate < 0) s->rate = 1000;
    lua_pop(L, 1);
    
    return 0;
}

/* This is the actual script thread function */
int
lua_script_thread(script_t *s)
{
    int func_ref;
    lua_State *L;
    struct timeval start, end;
    
    /* Create a lua interpreter object */
    L = luaL_newstate();
    setup_interpreter(L);    
    /* load and compile the file */
    if(luaL_loadfile(L, s->filename) ) {
        dax_error(ds, "Error Loading Main Script - %s", lua_tostring(L, -1));
        return 1;
    }
    /* Basicaly stores the Lua script */
    func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    /* Main Infinite Loop */
    while(1) {
        if(s->enable) {
            gettimeofday(&start, NULL);
            
            /* retrieve the funciton and put it on the stack */
            lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
            
            /* Get the configured tags and set the globals for the script */
            if(_receive_globals(L, s)) {
                dax_error(ds, "Unable to find all the global tags\n");
            } else {
                /* Run the script that is on the top of the stack */
                if( lua_pcall(L, 0, 0, 0) ) {
                    dax_error(ds, "Error Running Script - %s", lua_tostring(L, -1));
                }
                /* Write the configured global tags out to the server */
                /* TODO: Should we do something if this fails, if register globals
                   works then this should too */
                _send_globals(L, s);
                s->executions++;
                s->firstrun = 0;
            }
            
            /* This calculates the length of time that it took to run the script
             and then subtracts that time from the rate and calls usleep to hold
             for the right amount of time.  It ain't real time. */
            gettimeofday(&end, NULL);
            s->lastscan = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec/1000 - start.tv_usec/1000);
            /* If it takes longer than the scanrate then just go again instead of sleeping */
            if(s->lastscan < s->rate)
                usleep((s->rate - s->lastscan) * 1000);
        } else {
            usleep(s->rate * 1000);
        }
    }    
    
    /* Clean up and get out */
    lua_close(L);
    
    /* Should never get here */
    return 1;
}


/* This function attempts to start the thread given by s */
static int
_start_thread(script_t *s)
{    
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    if(pthread_create(&s->thread, &attr, (void *)&lua_script_thread, (void *)s)) {
        dax_error(ds, "Unable to start thread for script - %s", s->name);
        return -1;
    } else {
        dax_debug(ds, LOG_MAJOR, "Started Thread for script - %s", s->name);
        return 0;
    }
}

int
start_all_threads(void)
{
    script_t *s;
    int n, scount;
    
    scount = get_scriptcount();
    if(scount) {
        for(n = 0; n < scount; n++) {
            s = get_script(n);
            if(s) {
                _start_thread(s);
            }
        }
    }
    return 0;
}


int
main(int argc, char *argv[])
{
    struct sigaction sa;
    
    
    ds = dax_init("daxlua");
    if(ds == NULL) {
        fprintf(stderr, "Unable to Allocate DaxState Object\n");
        return ERR_ALLOC;
    }
    daxlua_init();
    
    /* For now */
    dax_set_debug_topic(ds, 0xFFFFFFFF);
    
    /* Reads the configuration */
    if(configure(argc,argv)) {
        dax_fatal(ds, "Unable to configure");
    }
    
    /* Check for OpenDAX and register the module */
    if( dax_mod_register(ds, "daxlua") ) {
        dax_fatal(ds, "Unable to find OpenDAX");
    }

    daxlua_set_state(NULL, ds);
    /* Run the initialization script */
    if(lua_init()) {
        dax_fatal(ds, "Init Script \'%s\' failed to run properly", get_init());
    } else {
        /* Start all the script threads */
        start_all_threads();
    }
    
    /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    /* TODO: How 'bout a SIGHUP to restart the module */
    
    
    while(1) {
        /* TODO: Probably should replace with a condition variable? */
        sleep(1);
        
        if(quitsig) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", quitsig);
            dax_mod_unregister(ds);
            if(quitsig == SIGQUIT) {
                exit(0);
            } else {
                exit(-1);    
            }
        }
        /* TODO: Check stuff, do stuff */
    }
    
    /* Should never get here */
    return 0;
}


/* this handles stopping the program */
void
quit_signal(int sig)
{
    quitsig = sig;
}
