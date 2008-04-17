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

#include <options.h>
#include <message.h>
#include <tagbase.h>
#include <common.h>
#include <module.h>
#include <func.h>
#include <pthread.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

extern struct Config config;

static void messagethread(void);
void child_signal(int);
void quit_signal(int);
void catch_signal(int);

int main(int argc, const char *argv[]) {
    struct sigaction sa;
    pthread_t message_thread;
	int result;
    
    //openlog("OpenDAX",LOG_NDELAY,LOG_DAEMON);
    xlog(0,"OpenDAX started");

    /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    //sa.sa_handler = &catch_signal;
    //sigaction(SIGHUP, &sa, NULL);

    set_log_topic(LOG_MAJOR); /*TODO: Needs to be configuration */
    set_log_topic(-1); /*TODO: Needs to be configuration */
    
    /* Read configuration from defaults, file and command line */
    opt_configure(argc, argv);
    /* Go to the background */
    if(opt_daemonize()) {
        if(daemonize("OpenDAX")) {
            xerror("Unable to go to the background");
        }
    }
    
    sa.sa_handler = &child_signal;
    sigaction (SIGCHLD, &sa, NULL);
    
    result = msg_setup();    /* This creates and sets up the message sockets */
    if(result) xerror("msg_setup() returned %d", result);
	initialize_tagbase(); /* initiallize the tagname list and database */
    
	/* Start the message handling thread */
    if(pthread_create(&message_thread,NULL,(void *)&messagethread,NULL)) {
        xfatal("Unable to create message thread");
    }
        
    module_start_all(); /* Start all the modules */
    
    /* DO TESTING STUFF HERE */

    /* END TESTING STUFF */
    
    while(1) { /* Main loop */
        /* TODO: This might could be some kind of condition
           variable or signal thing instead of just the sleep(). */
        module_scan();
        sleep(1);
    }
}

/* TODO: Does this really need to be a separate thread. */
/* This is the main message handling thread.  It should never return. */
static void messagethread(void) {
    while(1) {
        /* TODO: How far up should errors be passed */
        if(msg_receive()) sleep(1);
    }
}

/* This handles the shutting down of a module */
void child_signal(int sig) {
    int status;
    pid_t pid;

    do {
        pid = waitpid(-1, &status, WNOHANG);
        if(pid > 0) { 
        //xlog(1,"Caught Child Dying %d",pid);
            module_dmq_add(pid,status);
        }
    } while(pid > 0);
}

/* this handles shutting down of the core */
void quit_signal(int sig) {
    xlog(0,"Quitting due to signal %d", sig);
    /* TODO: Should stop all running modules */
    kill(0, SIGTERM); /* ...this'll do for now */
    /* TODO: Should verify that all the children are dead.  If not
       then send a SIGKILL signal (don't block in here though) */
    msg_destroy(); /* Destroy the message queue */
    exit(-1);
}

void catch_signal(int sig) {
    xlog(0, "Received signal %d", sig);
    
}
