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

#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <dax/func.h>
#include <modopt.h>
#include <modbus.h>

extern struct Config config;

void catchsignal(int sig);
void catchpipe(int sig);

static void getout(int);

void outdata(struct mb_port *,u_int8_t *,unsigned int);
void indata(struct mb_port *,u_int8_t *,unsigned int);

int main (int argc, const char * argv[]) {
    int result, n;
    struct mb_port *mp;
    struct mb_cmd *mc;
    struct sigaction sa;
    
    /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &catchsignal;
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sa.sa_handler = &catchpipe;
    sigaction(SIGPIPE, &sa, NULL);
    
    
    dax_debug(LOG_MAJOR, "Modbus Starting");
    /* Read the configuration from the command line and the file.
       Bail if there is an error. */
    result = modbus_configure(argc, argv);
    
    if( dax_mod_register("modbus") ) {
        dax_fatal("Unable to connect to OpenDAX server!");
    }
    
    /* Set the input and output callbacks if we aren't going to the background */
    /* TODO: Configuration option for these??? */
    for(n = 0; n < config.portcount; n++) {
        mb_set_output_callback(&config.ports[n], outdata);
        mb_set_input_callback(&config.ports[n], indata);
        
        mc = config.ports[n].commands;
        while(mc != NULL) {
            mc->idx = dt_add_tag(mc->tagname, mc->index, mc->function, mc->length);
            /* TODO: for now if we can't add the tag we'll keep the handle at zero.  The database
             * routines won't read or write if the handle is zero.  We should do more */
            if(mc->idx < 0) {
                mc->idx = 0;
            }
            mc = mc->next; /* get next command from the linked list */
        }
    }
    
    while(1) { /* Infinite loop, threads are doing all the work.*/
        /* Starts the port threads if they are not running */
        for(n=0; n < config.portcount; n++) {
            mp = &config.ports[n];
            if(!mp->running && !mp->inhibit) {
                if(mb_start_port(mp) < 0) { /* If the port fails to open */
                    mp->inhibit = 1; /* then don't try to open it anymore */
                    mp->inhibit_temp = 0; /* reset the inhibit timer */
                }
            } else if(mp->inhibit && mp->inhibit_time) {
                mp->inhibit_temp++;
                if(mp->inhibit_temp >= mp->inhibit_time) {
                    mp->inhibit = 0;
                    mp->inhibit_temp = 0;
                }
            }
/* TODO: The inhibition above could be tied to a timer or counter so that it would only be tried
every so often instead of being inhibited completely.  This would make it possible for the program to
recover from a USB serial port being pulled and then replaced or a device server going offline. */
        }
        sleep(1);
    }            
    return 0;
}


/* TODO: We need to have some kind of watchdog thread that accepts all
   the signals for the module and then deals with them appropriately.
   There also needs to be a wakeup routine for the port threads somehow. */

/* This function is used as the signal handler for all the signals
* that are going to be caught by the program */
void catchsignal(int sig) {
    if(sig == SIGHUP) {
        dax_log("Should be Reconfiguring Now");
        //--reconfigure();
    } else if(sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        dax_log("Exiting with signal %d", sig);
        getout(0);
    } else if(sig == SIGCHLD) {
        dax_log("Got SIGCHLD");
       /*There is probably some really cool child process handling stuff to do here
         but I don't quite know what to do yet. */
     } else if(sig == SIGUSR1) {
        dax_log("Got SIGUSR1");
    }
}

void
catchpipe(int sig)
{
    int n;
    
    for(n = 0; n < config.portcount; n++) {
        if(pthread_equal(pthread_self(), config.ports[n].thread)) {
            config.ports[n].dienow = 1;
        }
    }
}

static void
getout(int exitcode)
{
    int n;
    dax_debug(1, "Modbus Module Exiting");
    dax_mod_unregister();
    //--dt_destroy();
    
    /* Delete the PID file */   
    for(n = 0; n < config.portcount; n++) {
    /* TODO: Should probably stop the running threads here and then close the ports */
        close(config.ports[n].fd);
    }
    exit(exitcode);
}

/* Callback functions for printing the serial traffic */
void
outdata(struct mb_port *mp,u_int8_t *buff,unsigned int len)
{
   int n;
   for(n=0;n<len;n++) {
       printf("(%X)",buff[n]);
   }
   printf("\n");
}

void
indata(struct mb_port *mp,u_int8_t *buff,unsigned int len)
{
   int n;
   for(n=0;n<len;n++) {
       printf("[%X]",buff[n]);
   }
   printf("\n");
}
