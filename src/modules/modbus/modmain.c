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
#include <modopt.h>
#include <database.h>
#include <modbus.h>

extern struct Config config;
/* For now we'll keep ds as a global to simplify the code.  At some
 * point when the luaif library is complete it'll be stored in the
 * Lua_State Registry. */
dax_state *ds;
static int _caught_signal;

void catchsignal(int sig);
void catchpipe(int sig);

static void getout(int);

void outdata(mb_port *,uint8_t *,unsigned int);
void indata(mb_port *,uint8_t *,unsigned int);

/* Silly wrapper for calling the mb_run_port() function
 * Might add some housekeeping stuff later */
static void
_port_thread(void *port) {
    mb_run_port((mb_port *)port);
}


/* This callback function is assigned as the slave_write callback to the modbus
 * library.  After the library updates the modbus tables with new information this
 * function is called which writes the data back to the OpenDAX server */
static void
_slave_write_callback(mb_port *port, int reg, int index, int count, uint16_t *data)
{
    int result;
    tag_handle h;

    /* We're going to cheat and build our own tag_handle */
    /* We're assuming that the server loop won't call this function
     * with bad data. */
    switch(reg) {
        case MB_REG_HOLDING: /* These are the 16 bit registers */
            h.index = port->hold_tag.index;
            h.byte = index * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_INPUT:
            h.index = port->input_tag.index;
            h.byte = index * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_COIL:
            h.index = port->coil_tag.index;
            h.byte = index / 8;
            h.bit = index % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
        case MB_REG_DISC:
            h.index = port->disc_tag.index;
            h.byte = index / 8;
            h.bit = index % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
    }

    result = dax_write_tag(ds, h, data);
    if(result) {
        dax_error(ds, "Unable to write tag data to server\n");
    }
}


static void
_slave_read_callback(mb_port *port, int reg, int index, int count, uint16_t *data) {
    int result;
    tag_handle h;

    /* We're going to cheat and build our own tag_handle */
    /* We're assuming that the server loop won't call this function
     * with bad data. */
    switch(reg) {
        case MB_REG_HOLDING: /* These are the 16 bit registers */
            h.index = port->hold_tag.index;
            h.byte = index * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_INPUT:
            h.index = port->input_tag.index;
            h.byte = index * 2;
            h.bit = 0;
            h.count = count;
            h.size = count * 2;
            h.type = DAX_UINT;
            break;
        case MB_REG_COIL:
            h.index = port->coil_tag.index;
            h.byte = index / 8;
            h.bit = index % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
        case MB_REG_DISC:
            h.index = port->disc_tag.index;
            h.byte = index / 8;
            h.bit = index % 8;
            h.count = count;
            h.size = (h.bit + count - 1) / 8 - (h.bit / 8) + 1;
            h.type = DAX_BOOL;
            break;
    }
    result = dax_read_tag(ds, h, data);
    if(result) {
        dax_error(ds, "Unable to write tag data to server\n");
    }
}


/* When this function is called the port registers in the modbus library
 * have been setup but the OpenDAX part hasn't.  This function gets the
 * information from the port and sets up the tags in opendax */
static int
_setup_port(mb_port *port)
{
    unsigned int size;
    int result;

    // TODO: check for NULL tagname strings and errors and deal with them appropriately
    if(mb_get_port_type(port) == MB_SLAVE) {
        size = mb_get_holdreg_size(port);
        if(size) {
            result = dax_tag_add(ds, &port->hold_tag, port->hold_name, DAX_UINT, size, 0);
            if(result) dax_error(ds, "Failed to add holding register tag for port %s", port->name);
        }
        size = mb_get_inputreg_size(port);
        if(size) {
            result = dax_tag_add(ds, &port->input_tag, port->input_name, DAX_UINT, size, 0);
            if(result) dax_error(ds, "Failed to add input register tag for port %s", port->name);
        }
        size = mb_get_coil_size(port);
        if(size) {
            result = dax_tag_add(ds, &port->coil_tag, port->coil_name, DAX_BOOL, size, 0);
            if(result) dax_error(ds, "Failed to add coil register tag for port %s", port->name);
        }
        size = mb_get_discrete_size(port);
        if(size) {
            result = dax_tag_add(ds, &port->disc_tag, port->disc_name, DAX_BOOL, size, 0);
            if(result) dax_error(ds, "Failed to add discrete input tag for port %s", port->name);
        }
        mb_set_slave_write_callback(port, _slave_write_callback);
        mb_set_slave_read_callback(port, _slave_read_callback);
    }
    return 0;
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

    ds = dax_init("daxmodbus");
    if(ds == NULL) {
        fprintf(stderr, "Unable to Allocate DaxState Object\n");
        return ERR_ALLOC;
    }

    dax_set_debug_topic(ds, -1);
    dax_debug(ds, LOG_MAJOR, "Modbus Starting");
    /* Read the configuration from the command line and the file.
       Bail if there is an error. */
    result = modbus_configure(argc, argv);
    if(result) {
        exit(-1);
    }

    if( dax_connect(ds) ) {
        dax_fatal(ds, "Unable to connect to OpenDAX server!");
    }

    config.threads = malloc(sizeof(pthread_t) * config.portcount);
    if(config.threads == NULL) {
        dax_fatal(ds, "Unable to allocate memory for port threads!");
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for(n = 0; n < config.portcount; n++) {
        if(_setup_port(config.ports[n])) {
            dax_error(ds, "Problem setting up port - %s", mb_get_port_name(config.ports[n]));
        } else {
            mb_set_msgout_callback(config.ports[n], outdata);
            mb_set_msgin_callback(config.ports[n], indata);
            if(pthread_create(&config.threads[n], &attr, (void *)&_port_thread, (void *)config.ports[n])) {
                dax_error(ds, "Unable to start thread for port - %s", mb_get_port_name(config.ports[n]));
            } else {
                dax_debug(ds, LOG_MAJOR, "Started Thread for port - %s", mb_get_port_name(config.ports[n]));
            }
        }
    }
    /* TODO: Need some kind of semaphore here to signal when we can go to the running state */
    /* This might be a problem since the threads may not have actually started yet */
    dax_mod_set(ds, MOD_CMD_RUNNING, NULL);

    while(1) {
        sleep(1);
        if(_caught_signal) {
            if(_caught_signal == SIGHUP) {
                dax_log(ds, "Should be Reconfiguring Now");
                //--reconfigure();
                _caught_signal = 0;
            } else if(_caught_signal == SIGTERM || _caught_signal == SIGINT || 
                      _caught_signal == SIGQUIT) {
                dax_log(ds, "Exiting with signal %d", _caught_signal);
                getout(0);
            } else if(_caught_signal == SIGCHLD) {
                dax_log(ds, "Got SIGCHLD");
                /* TODO: Figure out which thread quit and restart it */
                _caught_signal = 0;
               /*There is probably some really cool child process handling stuff to do here
                 but I don't quite know what to do yet. */
             } else if(_caught_signal == SIGUSR1) {
                dax_log(ds, "Got SIGUSR1");
                _caught_signal = 0;
            }
        }
        /*TODO: Should scan ports to make sure that they are all still running */
    }

    return 0;
}

/* This function is used as the signal handler for all the signals
 * that are going to be caught by the program */
void catchsignal(int sig) {
    _caught_signal = sig;
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
    dax_disconnect(ds);

    for(n = 0; n < config.portcount; n++) {
    /* TODO: Should probably stop the running threads here and then close the ports */
        mb_close_port(config.ports[n]);
        mb_destroy_port(config.ports[n]);
    }
    exit(exitcode);
}

/* TODO: Change these to dax logging function */
/* Callback functions for printing the serial traffic */
void
outdata(mb_port *mp, uint8_t *buff, unsigned int len)
{
   int n;
   printf("%s:", mb_get_port_name(mp));
   for(n = 0; n < len; n++) {
       printf("(%X)", buff[n]);
   }
   printf("\n");
}

void
indata(mb_port *mp, uint8_t *buff, unsigned int len)
{
   int n;

   printf("%s:", mb_get_port_name(mp));
   for(n = 0; n < len; n++) {
       printf("[%X]",buff[n]);
   }
   printf("\n");
}
