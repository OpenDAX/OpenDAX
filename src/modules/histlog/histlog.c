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
#include <time.h>
#include <opendax.h>

#include "histlog.h"

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;
static unsigned int _flush_interval;
extern tag_config *tag_list;

static int
_test_difference(void *data1, void *data2, tag_type datatype, double threshold) {
    switch(datatype) {
        case DAX_BYTE:
            if(ABS(*(dax_byte *)data1 - *(dax_byte *)data2) >= threshold) return 1;
            else return 0;
        case DAX_SINT:
        case DAX_CHAR:
            if(ABS(*(dax_sint *)data1 - *(dax_sint *)data2) >= threshold) return 1;
            else return 0;
        case DAX_UINT:
        case DAX_WORD:
            if(ABS(*(dax_dint *)data1 - *(dax_dint *)data2) >= threshold) return 1;
            else return 0;
        case DAX_INT:
            if(ABS(*(dax_int *)data1 - *(dax_int *)data2) >= threshold) return 1;
            else return 0;
        case DAX_UDINT:
        case DAX_DWORD:
        case DAX_TIME:
            if(ABS(*(dax_udint *)data1 - *(dax_udint *)data2) >= threshold) return 1;
            else return 0;
        case DAX_DINT:
            if(ABS(*(dax_dint *)data1 - *(dax_dint *)data2) >= threshold) return 1;
            else return 0;
        case DAX_ULINT:
        case DAX_LWORD:
            if(ABS(*(dax_ulint *)data1 - *(dax_ulint *)data2) >= threshold) return 1;
            else return 0;
        case DAX_LINT:
            if(ABS(*(dax_lint *)data1 - *(dax_lint *)data2) >= threshold) return 1;
            else return 0;
        case DAX_REAL:
            if(ABS(*(dax_real *)data1 - *(dax_real *)data2) >= threshold) return 1;
            else return 0;
        case DAX_LREAL:
            if(ABS(*(dax_lreal *)data1 - *(dax_lreal *)data2) >= threshold) return 1;
            else return 0;
    }
    return 0;
}


static
void _event_callback(dax_state *ds, void *udata) {
    tag_config *tag = (tag_config *)udata;
    uint8_t buff[8];
    int result;
    double now = hist_gettime();

    result = dax_event_get_data(ds, buff, 8);
    if(result<0) {
        dax_error(ds, "Unable to get event data: %d", result);
        result = dax_read_tag(ds, tag->h, buff);
        if(result) {
            dax_error(ds, "Unable to read tag: %s", tag->name);
            return;
        }
    }
    /* Check if we are old */
    if(tag->timeout > 0.0 && (now - tag->lasttimestamp) > tag->timeout) {
        /* Write a NULL 'timeout' seconds in the past */
        write_data(tag->tag, NULL, now-tag->timeout);
        /* Write the current value */
        write_data(tag->tag, buff, now);
        if(tag->trigger == ON_CHANGE) {
            memcpy(tag->lastvalue, buff, tag->h.size);
            memcpy(tag->cmpvalue, buff, tag->h.size);
            tag->lastgood = 1;
        }
        tag->lasttimestamp = now;
        return;
    }
    /* This is where we decide how to store the tag.  If the trigger is WRITE then
     * we simply store every value we get.  If the trigger is CHANGE then we do a calculation
     * to see if it's changed enough according to the threshold that is stored.  If it has not
     * changed enough to pass our threshold we store the value in the tag_config structure so
     * that we always have the last value before the one we store.  If we have changed enough then
     * we store that last value that we kept with it's timestamp and then store the current one
     * with a right now timestamp.  Then we set the last timestamp to zero so that we can use it
     * as an indicator not to do this again in case the very next update has also changed enough
     * to trigger the write. */
    if(tag->trigger == ON_WRITE) {
        write_data(tag->tag, buff, hist_gettime());
    } else if(tag->trigger == ON_CHANGE) {
        if(_test_difference(tag->cmpvalue, buff, tag->h.type, tag->trigger_value)) {
            /* We have changed enough */
            /* If lastgood is false then that means that we have had some writes
             * that hadn't changed enough. */
            if(! tag->lastgood) {
                write_data(tag->tag, tag->lastvalue, tag->lasttimestamp);
                /* This keeps us from duplicating data if we have enough change on
                 * the next event.*/
                tag->lastgood = 1;
            }
            /* write the current data */
            write_data(tag->tag, buff, now);
            /* store it for next time */
            memcpy(tag->cmpvalue, buff, tag->h.size);
        } else {
            /* Not quite enough change */
            /* We keep this value in case the next one we get has changed enough. Storing
             * this value in the database along with the next one will make the curve fit
             * more accurate */
            memcpy(tag->lastvalue, buff, tag->h.size);
            /* This tells the above code that we'll have to write the last value on the next
             * change */
            tag->lastgood = 0;
        }
    } else {
        DF("Bad Log Trigger");
    }
    tag->lasttimestamp = now;
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
    uint8_t buff[8];

    this = tag_list;
    while(this != NULL) {
        if(!this->status) {
            result = dax_tag_handle(ds, &this->h, this->name, 1);
            if(result == ERR_NOTFOUND) {
                failures++;
            } else if(result == 0) { /* We found the tag so we can add it to the system */
                this->status = 1; /* No matter what we don't mess with this tag again */
                if(IS_CUSTOM(this->h.type)) {
                    dax_error(ds, "Tag custom datatypes are not supported - %s", this->name);
                } else {
                    this->tag = add_tag(this->name, this->h.type, this->attributes);
                    /* Write a NULL to signify that we were down */
                    write_data(this->tag, NULL, hist_gettime());
                    /* Read the current tag data and write it to the plugin */
                    result = dax_read_tag(ds, this->h, buff);
                    if(result == 0) write_data(this->tag, buff, hist_gettime());
                    else dax_error(ds, "Unable to read tag %s", this->name);
                    if(this->trigger == ON_CHANGE) {
                        /* Allocate the memory that we use to figure out and stored changed data */
                        this->lastvalue = malloc(this->h.size);
                        if(this->lastvalue != NULL) bzero(this->lastvalue, this->h.size);
                        else dax_fatal(ds, "Unable to allocate memory for %s", this->name);
                        /* Since cmpvalue is the value that we use to do the change comparison
                         * we are going to allocate it and write it here */
                        this->cmpvalue = malloc(this->h.size);
                        if(this->cmpvalue != NULL) memcpy(this->cmpvalue, buff, this->h.size);
                        else dax_fatal(ds, "Unable to allocate memory for %s", this->name);
                    }

                    result = dax_event_add(ds, &this->h, EVENT_WRITE, NULL, &id, _event_callback, this, _event_free);
                    if(result) {
                        dax_error(ds, "Unable to add event for tag %s", this->name);
                    }
                    result = dax_event_options(ds, id, EVENT_OPT_SEND_DATA);
                    if(result) {
                        dax_error(ds, "Unable to set event to send data");
                    }
                }
            } else {
                DF("Error %d", result);
            }
        }
        this = this->next;
    }
    return failures;
}

int
main(int argc,char *argv[]) {
    struct sigaction sa;
    int result;
    uint32_t loopcount = 0, interval = 1;
    int tag_failures = -1;
    struct timespec ts;
    time_t lasttime=0;

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
        dax_log(LOG_FATAL, "Unable to Allocate DaxState Object\n");
    }

    histlog_configure(argc, argv);

    /* Set the logging flags to show all the messages */
    dax_set_debug_topic(ds, LOG_ALL);

    result = plugin_load(dax_get_attr(ds, "plugin"));
    if(result) {
        dax_fatal(ds, "Unable to load a plugin");
    }
    _flush_interval = (unsigned int)strtol(dax_get_attr(ds, "flush_interval"), NULL, 0);
    DF("flush interval set to %d", _flush_interval);
    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_log(LOG_FATAL, "Unable to find OpenDAX");
    }

    /* Let's say we're running */
    dax_mod_set(ds, MOD_CMD_RUNNING, NULL);
    lasttime = 0;

    while(1) {
        clock_gettime(CLOCK_REALTIME, &ts);
        if(ts.tv_sec > lasttime + _flush_interval) {
            if(tag_failures && loopcount % interval == 0) {
                tag_failures = _add_tags();
                interval += 1;
                if(interval > 12) interval=12;
            }
            flush_data();
            lasttime = ts.tv_sec;
            loopcount++;
        }
        /* The first time through and as long as we have some unfound tags
         * we'll keep looping through the list to add them to the system. */
        /* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        dax_event_wait(ds, 1000, NULL);
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
    tag_config *this;
    double time_now;

    /* Write NULLs to the database */
    time_now = hist_gettime();
    this = tag_list;
    while(this != NULL) {
        write_data(this->tag, NULL, time_now);
        this = this->next;
    }
    flush_data();
    dax_disconnect(ds);
    exit(exitstatus);
}
