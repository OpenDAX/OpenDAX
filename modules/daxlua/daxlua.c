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
#include <common.h>
#include <signal.h>
//#include <dax/func.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

static int quitsig = 0;

void quit_signal(int sig);



/* Sets up and runs the init script.  The init script runs once
   at module start */
int
lua_init(void)
{
    lua_State *L;
    
    dax_debug(LOG_MAJOR, "Starting init script - %s", get_init());
 /* Create a lua interpreter object */
    L = luaL_newstate();
    setup_interpreter(L);    
 /* load and compile the file */
    if(luaL_loadfile(L, get_init()) || lua_pcall(L, 0, 0, 0) ) {
        dax_error("Error Running Init Script - %s", lua_tostring(L, -1));
        return 1;
    }
    
    /* Clean up and get out */
    lua_close(L);

    return 0;
}

/* Sets up and runs the given script in a new thread. */
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
        dax_error("Error Loading Main Script - %s", lua_tostring(L, -1));
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
            
            /* TODO: Get all the global tags and set the globals for the script */
            
            /* Run the script that is on the top of the stack */
            if( lua_pcall(L, 0, 0, 0) ) {
                dax_error("Error Running Script - %s", lua_tostring(L, -1));
                //return 1;
            }
            
            
            /* This calculates the length of time that it took to run the script
             and then subtracts that time from the rate and calls usleep to hold
             for the right amount of time.  It ain't real time. */
            gettimeofday(&end, NULL);
            s->lastscan = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec/1000 - start.tv_usec/1000);
            /* If it takes longer than the scanrate then just go again instead of sleeping */
            if(s->lastscan < s->rate)
                usleep((s->rate - s->lastscan)*1000);
        } else {
            usleep(s->rate * 1000);
        }
    }    
    
    /* Clean up and get out */
    lua_close(L);
    
    /* Should never get here */
    return 1;
}


static int
_start_thread(script_t *s)
{    
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    if(pthread_create(&s->thread, &attr, (void *)&lua_script_thread, (void *)s)) {
        dax_error( "Unable to start thread for script - %s", s->name);
        return -1;
    } else {
        dax_debug(LOG_MAJOR, "Started Thread for script - %s", s->name);
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


int main(int argc, char *argv[]) {
    struct sigaction sa;
    
    /* Set up the signal handlers */
    memset (&sa,0,sizeof(struct sigaction));
    sa.sa_handler=&quit_signal;
    sigaction (SIGQUIT,&sa,NULL);
    sigaction (SIGINT,&sa,NULL);
    sigaction (SIGTERM,&sa,NULL);
    /* TODO: How 'bout a SIGHUP to restart the module */
    
    daxlua_init();
    
    /* For now */
    dax_set_debug_topic(0xFFFFFFFF);
    
    /* Reads the configuration */
    if(configure(argc,argv)) {
        dax_fatal("Unable to configure");
    }
    
    /* Check for OpenDAX and register the module */
    if( dax_mod_register("daxlua") ) {
        dax_fatal("Unable to find OpenDAX");
    }
    
    /* Run the initialization script */
    if(lua_init()) {
        dax_fatal("Init Script \'%s\' failed to run properly", get_init());
    }
    
    /* Here we go */
    start_all_threads();
    
    while(1) {
        sleep(1);
        
        if(quitsig) {
            dax_debug(LOG_MAJOR, "Quitting due to signal %d", quitsig);
            dax_mod_unregister();
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
void quit_signal(int sig) {
    quitsig = sig;
}
