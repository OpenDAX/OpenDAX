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

static void messagethread(void);
void child_signal(int);
void quit_signal(int);

int main(int argc, const char *argv[]) {
    struct sigaction sa;
    pthread_t message_thread;
    int temp;

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

    /* TODO: Whether to go to the background should be an option */
    //if(daemonize("OpenDAX")) {
    //    xerror("Unable to go to the background");
    //}

    setverbosity(10); /*TODO: Needs to be configuration */
    
    temp=msg_setup_queue();    /* This creates and sets up the message queue */
    xlog(10,"msg_setup_queue() returned %d",temp);

    initialize_tagbase(); /* initiallize the tagname list and database */
    /* Start the message handling thread */
    if(pthread_create(&message_thread,NULL,(void *)&messagethread,NULL)) {
        xfatal("Unable to create message thread");
    }
    
    // TODO: Module addition should be handled from the configuration file
    temp=module_add("lsmod","/bin/ls","-l",MFLAG_OPENPIPES);
    temp=module_add("test","/Users/phil/opendax/modules/test/test",NULL,0);
    temp=module_add("modbus","/Users/phil/opendax/modules/modbus/modbus","-C /Users/phil/opendax/etc/modbus.conf",0);
    
    // TODO: There should be one giant global module starter
    module_start(3);
    
    while(1) {
        xlog(10,"Main Loop Scan");
        module_scan();
        sleep(5);
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
