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
 *
 *  Main source code file for the OpenDAX Historical Logging module
 */

#include <signal.h>
#include <opendax.h>

#include "histlog.h"

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;
extern tag_config *tag_list;

static
void _event_callback(dax_state *ds, void *udata) {
    tag_config *tag = (tag_config *)udata;
    uint8_t buff[8];
    int result;

    result = dax_event_get_data(ds, buff, 8);
    if(result<0) {
        dax_error(ds, "Unable to get event data: %d", result);
        result = dax_read_tag(ds, tag->h, buff);
        if(result) {
            dax_error(ds, "Unable to read tag: %s", tag->name);
            return;
        }
    }
    for(int i = 0; i<8;i++) printf("[%X]", buff[i]);
    printf("\n");
    if(tag->trigger == ON_WRITE) {
        write_data(tag->tag, buff, hist_gettime());
    } else if(tag->trigger == ON_CHANGE) {
        DF("change event %s", tag->name);
    } else {
        DF("Bad Log Trigger");
    }
}

static
void _event_free(void *udada) {
    ;
}

static int
_add_tags(void) {
    int result;
    tag_config *this;
    int failures = 0;
    dax_id id;

    this = tag_list;
    while(this != NULL) {
        if(!this->status) {
            result = dax_tag_handle(ds, &this->h, this->name, 1);
            if(result == ERR_NOTFOUND) {
                failures++;
            } else if(result == 0) {
                if(IS_CUSTOM(this->h.type)) {
                    dax_error(ds, "Tag custom datatypes are not supported - %s", this->name);
                    this->status = 1; /* No sense in trying this again */
                }
                this->tag = add_tag(this->name, this->h.type, this->attributes);
                result = dax_event_add(ds, &this->h, EVENT_WRITE, NULL, &id, _event_callback, this, _event_free);
                if(result) {
                    dax_error(ds, "Unable to add event for tag %s", this->name);
                }
                result = dax_event_options(ds, id, EVENT_OPT_SEND_DATA);
                if(result) {
                    dax_error(ds, "Unable to set event to send data");
                }
                this->status = 1;
            } else {
                DF("Error %d", result);
            }
        }
        //printf("tagname = %s\n", this->name);
        this = this->next;
    }
    return failures;
}

int
main(int argc,char *argv[]) {
    struct sigaction sa;
    int result;
    uint32_t loopcount = 0, interval = 5;
    int tag_failures = -1;

    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("histlog");
    if(ds == NULL) {
        /* dax_fatal() logs an error and causes a quit
         * signal to be sent to the module */
        dax_fatal(ds, "Unable to Allocate DaxState Object\n");
    }

    histlog_configure(argc, argv);

    /* Set the logging flags to show all the messages */
    dax_set_debug_topic(ds, LOG_ALL);

    result = plugin_load(dax_get_attr(ds, "plugin"));
    if(result) {
        dax_fatal(ds, "Unable to load a plugin");
    }
    set_config("filename", "NameOfFile.csv");
    get_config("TestAttr");


    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_fatal(ds, "Unable to find OpenDAX");
    }


    /* Let's say we're running */
    dax_mod_set(ds, MOD_CMD_RUNNING, NULL);
    while(1) {
        /* The first time through and as long as we have some unfound tags
         * we'll keep looping through the list to add them to the system. */
        if(tag_failures && loopcount % interval == 0) {
            tag_failures = _add_tags();
            interval *= 2; /* Double the time that we do this again */
            if(interval > 120) interval=120; /* 2 minutes max */
        }
        /* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        dax_event_wait(ds, 1000, NULL);
        loopcount++;
    }

 /* This is just to make the compiler happy */
    return(0);
}

/* Signal handler */
void
quit_signal(int sig) {
    _quitsignal = sig;
}

/* We call this function to exit the program */
static void
getout(int exitstatus) {
    dax_disconnect(ds);
    exit(exitstatus);
}
