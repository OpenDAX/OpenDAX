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
 * 
 * This is the main starting point for the OpenDAX Modbus Module
 */

#include <common.h>
#include <opendax.h>

#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <dax/func.h>
#include <modopt.h>
#include <database.h>
#include <lib/modbus.h>

extern struct Config config;
/* For now we'll keep ds as a global to simplify the code.  At some
 * point when the luaif library is complete it'll be stored in the
 * Lua_State Registry. */
dax_state *ds;

void catchsignal(int sig);
void catchpipe(int sig);

static void getout(int);

void outdata(mb_port *,u_int8_t *,unsigned int);
void indata(mb_port *,u_int8_t *,unsigned int);

/* Silly wrapper for calling the mb_run_port() function
 * Might add some housekeeping stuff later */
static void
_port_thread(void *port) {
    int result;
    result = mb_run_port((mb_port *)port);
    dax_fatal(ds, "This Shouldn't Exit Here!");
}

int
main (int argc, const char * argv[]) {
    int result, n;
    struct sigaction sa;
    pthread_attr_t attr;

    
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
    
    ds = dax_init("daxlua");
    if(ds == NULL) {
        fprintf(stderr, "Unable to Allocate DaxState Object\n");
        return ERR_ALLOC;
    }
    
    dax_set_debug_topic(ds, -1);
    dax_debug(ds, LOG_MAJOR, "Modbus Starting");
    /* Read the configuration from the command line and the file.
       Bail if there is an error. */
    result = modbus_configure(argc, argv);
    
    result = init_database();
    if(result) {
        dax_fatal(ds, "Unable to initialize the database!");
    }
    
    if( dax_mod_register(ds, "modbus") ) {
        dax_fatal(ds, "Unable to connect to OpenDAX server!");
    }
    
    config.threads = malloc(sizeof(pthread_t) * config.portcount);
    if(config.threads == NULL) {
        dax_fatal(ds, "Unable to allocate memory for port threads!");
    }
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for(n = 0; n < config.portcount; n++) {
        mb_set_msgout_callback(config.ports[n], outdata);
        mb_set_msgin_callback(config.ports[n], indata);
        printf("Starting Thread for port - %s",mb_get_name(config.ports[n]));
        if(pthread_create(&config.threads[n], &attr, (void *)&_port_thread, (void *)config.ports[n])) {
            dax_error(ds, "Unable to start thread for port - %s", mb_get_name(config.ports[n]));
        } else {
            dax_debug(ds, LOG_MAJOR, "Started Thread for port - %s", mb_get_name(config.ports[n]));
        }
    }
    
    while(1) {
        sleep(10);
        /*TODO: May need to do some kind of housekeeping here */
    }
    
    for(n = 0; n < config.portcount; n++) {
        mb_close_port(config.ports[n]);
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
        dax_log(ds, "Should be Reconfiguring Now");
        //--reconfigure();
    } else if(sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        dax_log(ds, "Exiting with signal %d", sig);
        getout(0);
    } else if(sig == SIGCHLD) {
        dax_log(ds, "Got SIGCHLD");
       /*There is probably some really cool child process handling stuff to do here
         but I don't quite know what to do yet. */
     } else if(sig == SIGUSR1) {
        dax_log(ds, "Got SIGUSR1");
    }
}

void
catchpipe(int sig)
{
    int n;
    
    for(n = 0; n < config.portcount; n++) {
        //if(pthread_equal(pthread_self(), config.ports[n].thread)) {
            //mb_kill_port_thread(config.ports[n]);
        //}
    }
}

static void
getout(int exitcode)
{
    int n;
    dax_debug(ds, 1, "Modbus Module Exiting");
    dax_mod_unregister(ds);
    
    for(n = 0; n < config.portcount; n++) {
    /* TODO: Should probably stop the running threads here and then close the ports */
        mb_destroy_port(config.ports[n]);
    }
    exit(exitcode);
}

/* Callback functions for printing the serial traffic */
void
outdata(mb_port *mp, u_int8_t *buff, unsigned int len)
{
   int n;
   printf("%s:", mb_get_name(mp));
   for(n = 0; n < len; n++) {
       printf("(%X)", buff[n]);
   }
   printf("\n");
}

void
indata(mb_port *mp, u_int8_t *buff, unsigned int len)
{
   int n;
   printf("%s:", mb_get_name(mp));
   for(n = 0; n < len; n++) {
       printf("[%X]",buff[n]);
   }
   printf("\n");
}
