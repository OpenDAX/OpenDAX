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
 *  Main source code file for the OpenDAX Historical Logging module
 */

#include <signal.h>
#include <opendax.h>

#include "histlog.h"

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;

/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;

    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("histlog");
    if(ds == NULL) {
        /* dax_fatal() logs an error and causes a quit
         * signal to be sent to the module */
        dax_fatal(ds, "Unable to Allocate DaxState Object\n");
    }

    histlog_configure(argc, argv);

    /* Set the logging flags to show all the messages */
    dax_set_debug_topic(ds, LOG_ALL);

    plugin_load("plugins/file/libdhl_file.so");
    add_tag("NameOfTheTag", 1, NULL);
    write_data(42, NULL, 0.0);
    set_config("filename", "NameOfFile.csv");
    get_config("TestAttr");

    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_fatal(ds, "Unable to find OpenDAX");
    }
    /* Let's say we're running */
    dax_mod_set(ds, MOD_CMD_RUNNING, NULL);
    while(1) {
        /* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }

        sleep(1);
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
