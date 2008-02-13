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
#include <opendax.h>
#include <common.h>
#include <signal.h>
#include <options.h>
#include <dax/func.h>
#include <string.h>
#include <sys/time.h>
#include <luaif.h>

void quit_signal(int sig);
int lua_init(void);
int lua_main(void);


int main(int argc, char *argv[]) {
    struct sigaction sa;
    
 /* Set up the signal handlers */
    memset (&sa,0,sizeof(struct sigaction));
    sa.sa_handler=&quit_signal;
    sigaction (SIGQUIT,&sa,NULL);
    sigaction (SIGINT,&sa,NULL);
    sigaction (SIGTERM,&sa,NULL);
    /* TODO: How 'bout a SIGHUP to restart the module */

 /* Reads the configuration */
    if(configure(argc,argv)) {
        xfatal("Unable to configure");
    }
    
 /* Check for OpenDAX and register the module */
    if( dax_mod_register("daxlua") ) {
        xfatal("Unable to find OpenDAX");
    }
    
 /* Run the initialization script */
    if(lua_init()) {
        xfatal("Init Script \'%s\' failed to run properly", get_init());
    }
 /* Start running the main script */    
    if(lua_main()) {
        xfatal("Main Script \'%s\' failed to run properly", get_main());
    }
 /* Should never get here */
    return 0;
}

/* Sets up and runs the init script.  The init script runs once
   at module start */
int lua_init(void) {
    lua_State *L;
    
 /* Create a lua interpreter object */
    L = luaL_newstate();
    setup_interpreter(L);    
 /* load and compile the file */
    if(luaL_loadfile(L, get_init()) || lua_pcall(L, 0, 0, 0) ) {
        xerror("Error Running Init Script - %s", lua_tostring(L, -1));
        return 1;
    }
    
    /* Clean up and get out */
    lua_close(L);

    return 0;
}

/* Sets up and runs the main script.  The main script runs
   every 'rate' mSec. for the life of the module */
int lua_main(void) {
    int func_ref;
    long rate, time_spent;
    lua_State *L;
    struct timeval start, end;
    
 /* Create a lua interpreter object */
    L = luaL_newstate();
    setup_interpreter(L);    
 /* load and compile the file */
    if(luaL_loadfile(L, get_main()) ) {
        xerror("Error Loading Main Script - %s", lua_tostring(L, -1));
        //--Don't bail on errors just yet
        //--return 1;
    }
 /* Basicaly stores the Lua script */
    func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
/* PARSE THE _G TABLE */    

    lua_getglobal(L, "_G");
    /* table is in the stack at index 't' */
    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        printf("%s - %s\n",
               //lua_typename(L, lua_type(L, -2)),
               lua_tostring(L, -2),
               lua_typename(L, lua_type(L, -1)));
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
    

/* */
    
    rate = get_rate();
 /* Main Infinite Loop */
    while(1) {
        gettimeofday(&start, NULL);
        
        /* retrieve the funciton and put it on the stack */
        lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
        
        /* Run the script that is on the top of the stack */
        if( lua_pcall(L, 0, 0, 0) ) {
            xerror("Error Running Main Script - %s", lua_tostring(L, -1));
            //return 1;
        }

        
     /* This calculates the length of time that it took to run the script
        and then subtracts that time from the rate and calls usleep to hold
        for the right amount of time.  It ain't real time. */
        gettimeofday(&end,NULL);
        time_spent = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec/1000 - start.tv_usec/1000);
     /* If it takes longer than the scanrate then just go again instead of sleeping */
        if(time_spent < rate)
            usleep((rate - time_spent)*1000);
    }    
    
 /* Clean up and get out */
    lua_close(L);
    
 /* Should never get here */
    return 1;
}

/* this handles stopping the program */
void quit_signal(int sig) {
    xlog(0, "Quitting due to signal %d", sig);
    dax_mod_unregister();
    if(sig == SIGQUIT) {
        exit(0);
    } else {
        exit(-1);    
    }
}
