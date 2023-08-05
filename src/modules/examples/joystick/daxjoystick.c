/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2021 Phil Birkelbach
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
 *  This is the main source file for the OpenDAX joystick module
 **/

#include "daxjoystick.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "linux/joystick.h"

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;
static int _runsignal;

axis_t axis_list[256];
button_t button_list[256];

/* TODO: check the number of axis and buttons and make sure that we don't have
 * an axis or button configured that doesn't exist and give errors otherwise */
static int
_open_device() {
    int result;
    /* Get the results of the configuration */
    result = open(dax_get_attr(ds, "device"), O_RDONLY);
    return result;
}

static int
read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}


void
_run_function(dax_state *ds, void *ud) {
    _runsignal = 1;
    dax_log(LOG_DEBUG, "Run signal received from tagserver");
    dax_default_run(ds, ud);
}

void
_stop_function(dax_state *ds, void *ud) {
    _runsignal = 0;
    dax_log(LOG_DEBUG, "Stop signal received from tagserver");
    dax_default_stop(ds, ud);
}

void
_kill_function(dax_state *ds, void *ud) {
    _quitsignal = 1;
    dax_log(LOG_MINOR, "Kill signal received from tagserver");
    dax_default_kill(ds, ud);
}


/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;
    int result = 0, fd, n;
    struct js_event event;
    dax_real val;

    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("joystick");
    if(ds == NULL) {
        /* dax_fatal() logs an error and causes a quit
         * signal to be sent to the module */
        dax_log(LOG_FATAL, "Unable to Allocate DaxState Object\n");
        kill(getpid(), SIGQUIT);
    }
    result = configure(argc, argv);
    if(result) {
        dax_log(LOG_FATAL, "Fatal error in configuration");
        exit(result);
    }

    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_log(LOG_FATAL, "Unable to find OpenDAX");
        kill(getpid(), SIGQUIT);
    }
    /* TODO: instead of creating the tags here, we might add some create_tag() routines and simply
     * get handles for the strings in the array.  If the tag doesn't exist then create a single tag
     * if it doesn't have an [ or a . in it.  This would let us write these values to CDTs and arrays
     * if they are already created.  For now we're just trying to make it work.
     */
    for(n=0;n<256;n++) { /* Loop through the axis list and create the tags */
        axis_list[n].handle.index = 0; /* to mark it so we don't try to write to it later */
        if(axis_list[n].tagname != NULL) {
            result = dax_tag_add(ds, &axis_list[n].handle, axis_list[n].tagname, DAX_REAL, 1, 0);
            if(result) {
                dax_log(LOG_ERROR, "Unable to add tag '%s'", axis_list[n].tagname);
            }
        }
    }
    for(n=0;n<256;n++) { /* Loop through the axis list and create the tags */
        button_list[n].handle.index = 0; /* to mark it so we don't try to write to it later */
        if(button_list[n].tagname != NULL) {
            result = dax_tag_add(ds, &button_list[n].handle, button_list[n].tagname, DAX_BOOL, 1, 0);
            if(result) {
                dax_log(LOG_ERROR, "Unable to add tag '%s'", button_list[n].tagname);
            }
        }
    }

    dax_set_running(ds, 1);
    _runsignal = 1;
    dax_set_run_callback(ds, _run_function);
    dax_set_stop_callback(ds, _stop_function);
    dax_set_kill_callback(ds, _kill_function);
    dax_set_status(ds, "OK");
    
    dax_log(LOG_MAJOR,"Joystick Module Starting?????");

    while(1) { /* device file open loop */
        if(_runsignal) {
            fd = _open_device();
            if(fd > 0) {
                dax_log(LOG_MINOR, "Connected to joystick at fd = %d", fd);
                while(_runsignal && read_event(fd, &event) == 0 && ! _quitsignal) {
                    switch (event.type)
                    {
                        case JS_EVENT_BUTTON:
                            if(button_list[event.number].handle.index) {
                                dax_write_tag(ds, button_list[event.number].handle, &event.value);
                            }
                            break;
                        case JS_EVENT_AXIS:
                            if(axis_list[event.number].handle.index) {
                                /* Transfer function (axis + offset) * scale */
                                val = (event.value + axis_list[event.number].offset) + axis_list[event.number].scale;
                                dax_write_tag(ds, axis_list[event.number].handle, &val);
                            }
                            break;
                        default:
                            /* Ignore init events. */
                            break;
                    }
                /* This is so that our stop/run/kill events will get to us.
                   A better way to do this would be to have a thread of it's own
                   but since this is just an example we'll do it here */
                dax_event_poll(ds, NULL);
                }
            }
            dax_log(LOG_MINOR, "Disconnecting from joystick at fd = %d", fd);
            close(fd);
        }
        /* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_log(LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        dax_event_wait(ds, 1000, NULL);
    }
    getout(-1);
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
