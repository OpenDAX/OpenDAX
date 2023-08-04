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

#define _GNU_SOURCE
#include "daxlua.h"
#include <signal.h>
#include <pthread.h>

static int quitsig = 0;
dax_state *ds;
int runsignal = 1;

void quit_signal(int sig);

static void
run_function(dax_state *ds, void *ud) {
    runsignal = 1;
    dax_default_run(ds, ud);
}

static void
stop_function(dax_state *ds, void *ud) {
    runsignal = 0;
    dax_default_stop(ds, ud);
}

static void
kill_function(dax_state *ds, void *ud) {
    dax_log(LOG_MAJOR, "Recieved kill signal from tagserver");
    quitsig = 1;
    dax_default_kill(ds, ud);
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

    /* Reads the configuration */
    if(configure(argc,argv)) {
        dax_log(LOG_FATAL, "Unable to configure");
        return -1;
    }

    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_log(LOG_FATAL, "Unable to find OpenDAX");
        kill(getpid(), SIGQUIT);
    }

    daxlua_set_state(NULL, ds);
    /* Start all the script threads */
    start_all_scripts();

    /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    dax_set_running(ds, 1);
    dax_set_run_callback(ds, run_function);
    dax_set_stop_callback(ds, stop_function);
    dax_set_kill_callback(ds, kill_function);

    while(1) {
        dax_event_wait(ds, 1000, NULL);

        if(quitsig) {
            dax_log(LOG_MAJOR, "Quitting due to signal %d", quitsig);
            dax_disconnect(ds);
            if(quitsig == SIGQUIT) {
                exit(0);
            } else {
                exit(-1);
            }
        }
        // TODO: Update thread status tags (overruns, scan times, etc)
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
