/* modmain.c Modbus Communications Module for OpenDAX
 * Copyright (C) 2007 Phil Birkelbach
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <common.h>
#include <opendax.h>

#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <dax/func.h>
#include <options.h>
#include <modbus.h>

extern struct Config config;

//static void daemonize(void);
//static void writepidfile(void);
void catchsignal(int sig);
static void getout(int);

void outdata(struct mb_port *,u_int8_t *,unsigned int);
void indata(struct mb_port *,u_int8_t *,unsigned int);

int main (int argc, const char * argv[]) {
    int result,n,time;
    /* Set up the signal handler */
    //TODO: Should use sigaction() instead??
    (void)signal(SIGHUP,catchsignal);
    (void)signal(SIGINT,catchsignal);
    (void)signal(SIGTERM,catchsignal);
    (void)signal(SIGPIPE,catchsignal);
    (void)signal(SIGQUIT,catchsignal);
    /* Probably should use this to see if we've lost a port thread */
    (void)signal(SIGCHLD,SIG_IGN);
    
    /* Set up the system logger */
    /*TODO: Use the OpenDAX logger */
    //openlog("modbus", LOG_NDELAY,LOG_DAEMON);
    dax_debug(1, "Modbus Starting");
   
    /* Read the configuration from the command line and the file.
       Bail if there is an error. */
    result = modbus_configure(argc,argv);
    
    if( dax_mod_register("modbus") ) {
        dax_log("We got's to go");
        dax_fatal("Unable to find OpenDAX Message Queue.  OpenDAX probalby not running");
    }
    
    /* Allocate and initialize the datatable */
    time = 1; n = 0;
    while(dt_init(config.tablesize)) {
        syslog(LOG_ERR, "Unable to allocate the data table");
        sleep(time);
        if(n++ > 5) {
            time *= 2; n = 0;
        }
        if(time > 60) time = 60;
    }
    
    /* If configured daemonize the program */
    //if(config.daemonize) {
    //    daemonize("modbus");
    //} else {
    /* Set the input and output callbacks if we aren't going to the background */
        for(n = 0; n < config.portcount; n++) {
            mb_set_output_callback(&config.ports[n], outdata);
            mb_set_input_callback(&config.ports[n], indata);
        }
    //}
    /* Start the threads that handle the sockets */
    
    while(1) { /* Infinite loop, threads are doing all the work.*/
        /* Starts the port threads if they are not running */
        for(n=0;n<config.portcount;n++) {
            if(!config.ports[n].running && !config.ports[n].inhibit)
                if(mb_start_port(&config.ports[n]) < 0) /* If the port fails to open */
                    config.ports[n].inhibit=1; /* then don't try to open it anymore */
/* TODO: The inhibition above could be tied to a timer or counter so that it would only be tried
every so often instead of being inhibited completely.  This would make it possible for the program to
recover from a USB serial port being pulled and then replaced or a device server going offline. */
        }
        sleep(1);
    }            
    return 0;
}


/* This funciton daemonizes the program. */
#ifdef SLKJFOIENOIBOEOIHFWELKJ
static void daemonize(void) {
    pid_t result;
    int n;
    char s[10];
   /* Replace with xlog() */
    if(config.verbose) syslog(LOG_NOTICE,"Daemonizing the process");
    /* Call fork() and exit as the parent.  This returns control to the 
       command line and guarantees the program is not a process group
       leader. */
    result=fork();
    if(result<0) {
        syslog(LOG_ERR,"Failed initial process creation");
        getout(-1);
    } else if(result>0) {
        exit(0);
    }

    /*Call setsid() to dump the controlling terminal*/
    result=setsid();
   
    if(result<0) {
        syslog(LOG_ERR,"Unable to dump controling terminal");
        getout(-1);
    }

    /* Call fork() and exit as the parent.  This assures that we
       will never again be able to connect to a controlling 
       terminal */
    result=fork();
    if(result<0) {
        syslog(LOG_ERR,"Unable to fork the final fork");
        getout(-1);
    } else if(result>0)
        exit(0);
   
    /* chdir() to / so that we don't hold any directories open */
    chdir("/");
    /* need control of the permissions of files that we write */
    umask(0);
    /* writes the PID to STDOUT */
    sprintf(s,"%d",getpid());
    write(2,s,strlen(s));
    writepidfile();
    
    /* Closes the logger before we pull the rug out from under it
       by closing all the file descriptors */
    closelog();
    /* close all open file descriptors */
    for (n=getdtablesize();n>=0;--n) close(n);
    /*reopen stdout stdin and stderr to /dev/null */
    n = open("/dev/null",O_RDWR);
    dup(n); dup(n);
    /* reopen the logger */
    openlog("mbd",LOG_NDELAY,LOG_DAEMON);
}

/* Writes the PID to the pidfile if one is configured */
static void writepidfile(void) {
    int pidfd=0;
    char pid[10];
    umask(0);
    pidfd=open(config.pidfile,O_RDWR | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
    if(!pidfd) 
        dax_log("Unable to open PID file - %s",config.pidfile);
    else {
        sprintf(pid,"%d",getpid());
        write(pidfd,pid,strlen(pid)); /*writes to the PID file*/
    }
    unlink(config.pidfile);
}
#endif

/* This function is used as the signal handler for all the signals
* that are going to be caught by the program */
void catchsignal(int sig) {
    if(sig == SIGHUP) {
        dax_log("Should be Reconfiguring Now");
        //reconfigure();
    } else if(sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        dax_log("Exiting with signal %d", sig);
    getout(0);
    } else if(sig == SIGCHLD) {
        dax_log("Got SIGCHLD");
       /*There is probably some really cool child process handling stuff to do here
         but I don't quite know what to do yet. */
    } else if(sig == SIGPIPE) {
        /* TODO: We should shutdown or restart a port or something here */
        dax_log("Got SIGPIPE");
    }
}


static void getout(int exitcode) {
    int n;
    syslog(LOG_NOTICE, "Process exiting");
    closelog();
    
    dt_destroy();
    /* TODO: Need to figure out a way to unbind the socket */
    
    /* Delete the PID file */   
    for(n=0; n<config.portcount; n++) {
    /* TODO: Should probably stop the running threads here and then close the ports */
        close(config.ports[n].fd);
    }
    exit(exitcode);
}

/* Callback functions for printing the serial traffic */
void outdata(struct mb_port *mp,u_int8_t *buff,unsigned int len) {
   int n;
   for(n=0;n<len;n++) {
       printf("(%X)",buff[n]);
   }
   printf("\n");
}

void indata(struct mb_port *mp,u_int8_t *buff,unsigned int len) {
   int n;
   for(n=0;n<len;n++) {
       printf("[%X]",buff[n]);
   }
   printf("\n");
}
