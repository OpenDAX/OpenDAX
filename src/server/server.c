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
 */

#define _XOPEN_SOURCE
#include <common.h>
#include "options.h"
#include "message.h"
#include "tagbase.h"
#include "retain.h"
#include "func.h"
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <sys/wait.h>

static int quitflag = 0;

static void messagethread(void);
void quit_signal(int);
void catch_signal(int);

int
main(int argc, const char *argv[])
{
    struct sigaction sa;
    pthread_t message_thread;
	int result;

    /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    sa.sa_handler = &catch_signal;
    sigaction(SIGPIPE, &sa, NULL);

    dax_init_logger("tagserver", DAX_LOG_ERROR | DAX_LOG_FATAL);
    /* Read configuration from defaults, file and command line */
    opt_configure(argc, argv);

    result = msg_setup();    /* This creates and sets up the message sockets */
    if(result) dax_log(DAX_LOG_ERROR, "msg_setup() returned %d", result);
    initialize_tagbase(); /* initialize the tag name database */
    /* TODO: Add retention filename from configuration */
    ret_init(NULL);
    /* Start the message handling thread */
    if(pthread_create(&message_thread, NULL, (void *)&messagethread, NULL)) {
        dax_log(DAX_LOG_FATAL, "Unable to create message thread");
        kill(getpid(), SIGQUIT);
    }

    dax_log(DAX_LOG_MAJOR, "OpenDAX Tag Server Started");

    while(1) { /* Main loop */
        sleep(10); /* A signal should interrupt this */
        /* If the quit flag is set then we clean up and get out */
        if(quitflag) {
            dax_log(DAX_LOG_MAJOR, "Quitting due to signal %d", quitflag);
            msg_destroy(); /* Clean up messaging code */
            ret_close();   /* Clean up tag retention system */
            exit(0);
        }
    }
}

/* This is the main message handling thread.  It should never return. */
static void
messagethread(void)
{
    int result;

    while(1) {
        result = msg_receive();
        if(result) {
            dax_log(DAX_LOG_ERROR, "Message received with error: %d", result);
            sleep(1);
        }
    }
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
    /* TODO: Shouldn't really have printfs in signal handlers */
    dax_log(DAX_LOG_MINOR, "Received signal %d", sig);
}
