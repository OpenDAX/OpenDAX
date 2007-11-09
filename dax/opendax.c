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

#include <common.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <syslog.h>
#include <module.h>
#include <options.h>
#include <func.h>
#include <pthread.h>
#include <message.h>
#include <tagbase.h>

extern struct Config config;

static void messagethread(void);
void child_signal(int);
void quit_signal(int);

int main(int argc, const char *argv[]) {
    struct sigaction sa;
    pthread_t message_thread;
        
    openlog("OpenDAX",LOG_NDELAY,LOG_DAEMON);
    xlog(0,"OpenDAX started");

    /* Set up the signal handlers */
    memset (&sa,0,sizeof(struct sigaction));
    sa.sa_handler=&child_signal;
    sigaction (SIGCHLD,&sa,NULL);
    sa.sa_handler=&quit_signal;
    sigaction (SIGQUIT,&sa,NULL);
    sigaction (SIGINT,&sa,NULL);
    sigaction (SIGTERM,&sa,NULL);

    /* TODO: This is the program architecture in a nutshell...
        check_already_running();
            if we are then do what the options say or possibly 
            re-run the configuration.
	 *? read_configuration();
      * daemonize();
        setverbosity();
      * create_message_queue();
	  * initialize_tagbase();
	  * start_message_thread();
        setup_slave / redundant sockets();
      * start_modules();
	  * start_main_loop();
    */
    
    setverbosity(10); /*TODO: Needs to be configuration */
    
    /* Read configuration from defaults, file and command line */
    dax_configure(argc, argv);
    /* Go to the background */
    if(config.daemonize) {
        if(daemonize("OpenDAX")) {
            xerror("Unable to go to the background");
        }
    }
    
    msg_setup_queue();    /* This creates and sets up the message queue */
    initialize_tagbase(); /* initiallize the tagname list and database */
    
	/* Start the message handling thread */
    if(pthread_create(&message_thread,NULL,(void *)&messagethread,NULL)) {
        xfatal("Unable to create message thread");
    }
    
    module_start_all(); /* Start all the modules */
    
    while(1) { /* Main loop */
        module_scan();
        sleep(1);
    }
}

/* This is the main message handling thread.  It should never return. */
static void messagethread(void) {
    /*TODO: So far this function just blocks to receive messages.  It should
    handle cases where the message reception was interrupted by signals and
    alarms.  Occasionally it should check the message queue for orphaned
    messages that may have been left by some misbehaving modules.  This
    algorithm should increase in frequency when it finds lost messages and
    decrease in frequency when it doesn't.*/
    while(1) {
        msg_receive();
    }
}

/* This handles the shutting down of a module */
void child_signal(int sig) {
    int status;
    pid_t pid;

    pid=wait(&status);
    xlog(1,"Caught Child Dying %d\n",pid);
    module_dmq_add(pid,status);
}

/* this handles shutting down of the core */
void quit_signal(int sig) {
    xlog(0,"Quitting due to signal %d",sig);
    /* TODO: Should stop all running modules */
    kill(0,SIGTERM); /* ...this'll do for now */
    msg_destroy_queue(); /* Destroy the message queue */
    exit(-1);
}
