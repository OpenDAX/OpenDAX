/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2012 Phil Birkelbach
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

/*  Source code file for the dax master process handling application */

#include "process.h"
#include "mstr_config.h"
#include "daemon.h"
#include <common.h>
#include <syslog.h>
#include <signal.h>
#include <sys/wait.h>
#include <opendax.h>

static int quitflag = 0;

void child_signal(int);
void quit_signal(int);
void catch_signal(int);

int
main(int argc, const char *argv[])
{
    struct sigaction sa;
    int restart;

    /* Set up the signal handlers */
    /* TODO: We need to handle every signal that could possibly kill us */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    sa.sa_handler = &catch_signal;
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_handler = &child_signal;
    sigaction (SIGCHLD, &sa, NULL);

    /* Set this first in case we need to output some data */
    /* If we ever open the logger with syslog instead, we'll have to
     * make sure and close the logger before we deamonize and then
     * call logger_init() again afterwards */
    process_init();
    /* TODO: We should have individual configuration objects that we retrieve
     * from this function, instead of the global data in the source file. */
    opt_configure(argc, argv);

    if(opt_daemonize()) {
        daemonize("opendax");
    }
    process_start_all();

    _print_process_list();

    while(1) { /* Main loop */
        /* TODO: This might could be some kind of condition
           variable or signal thing instead of just the sleep(). */
        restart = process_scan();
        if(restart < 1000) {
            usleep(restart * 1000);
        } else {
            sleep(1);
        }
        /* If the quit flag is set then we clean up and get out */
        if(quitflag) {
            dax_log(LOG_MAJOR, "Master quiting due to signal %d", quitflag);
            /* TODO: Should stop all running modules and wait for them
               to exit.  If they fail then kill -9 those rascals*/
            kill(0, SIGTERM); /* ...this'll do for now */
            closelog();
            exit(-1);
        }

    }
    exit(0);
}

/* this handles shutting down of the server */
/* TODO: There's the easy way out and then there is the hard way out.
 * I need to figure out which is which and then act appropriately.
 */
void
quit_signal(int sig)
{
    quitflag = sig;
}

/* TODO: May need to so something with signals like SIGPIPE etc */
void
catch_signal(int sig)
{
    // TODO: No printfs in signal handlers
    dax_log(LOG_MINOR, "Master received signal %d", sig);
}


/* Clean up any child modules that have shut down */
void
child_signal(int sig)
{
    int status;
    pid_t pid;

    do {
        pid = waitpid(-1, &status, WNOHANG);
        if(pid > 0) {
            process_dpq_add(pid, status);
        }
    } while(pid > 0);
}
