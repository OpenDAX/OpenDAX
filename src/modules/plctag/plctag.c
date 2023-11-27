/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2023 Phil Birkelbach
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

/* This is main source file for the the PLCTAG client module.  This module
 * uses the features of libplctag to read/write tags from supported PLCs.
 * See the documentation of libplctag at https://github.com/libplctag/libplctag
 * for supported PLCs.
 */

#define _GNU_SOURCE
#include "plctag.h"
#include <libplctag.h>
#include <signal.h>
#include <libdaxlua.h>

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;
static int _runsignal;
struct tagdef *taglist = NULL;

void
_run_function(dax_state *ds, void *ud) {
    _runsignal = 1;
    dax_log(DAX_LOG_DEBUG, "Run signal received from tagserver");
    dax_default_run(ds, ud);
}

void
_stop_function(dax_state *ds, void *ud) {
    _runsignal = 0;
    dax_log(DAX_LOG_DEBUG, "Stop signal received from tagserver");
    dax_default_stop(ds, ud);
}

void
_kill_function(dax_state *ds, void *ud) {
    _quitsignal = 1;
    dax_log(DAX_LOG_MINOR, "Kill signal received from tagserver");
    dax_default_kill(ds, ud);
}
/* This reads the data from teh plctag and converts it to the proper type for
   the opendax dat.  The conversions are made based on the type of opendax
   tag but the offset for the conversion in the plctag is calculated based
   on the plctag element size */
static void
_read_tag(struct tagdef *tag) {
    int offset;
    int result;
    dax_type_union val;
    uint8_t rawbuff[tag->elem_size];

    switch(tag->h.type) {
        case DAX_BOOL:
            for(offset = 0; offset < tag->h.count; offset++) {
                result = plc_tag_get_bit(tag->tag_id, offset);
                if(result > 0) {
                    tag->buff[offset/8] |= 0x01 << (offset % 8);
                } else if(result < 0) {
                    DF("Error"); // TODO: What to do with this error???
                } else {
                    tag->buff[offset/8] &= ~(0x01 << (offset % 8));
                }
            }
            break;
        case DAX_BYTE:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_byte = plc_tag_get_uint8(tag->tag_id, offset*tag->elem_size);
                ((dax_byte *)tag->buff)[offset] = val.dax_byte;
            }
            break;
        case DAX_SINT:
        case DAX_CHAR:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_sint = plc_tag_get_int8(tag->tag_id, offset*tag->elem_size);
                ((dax_sint *)tag->buff)[offset] = val.dax_sint;
            }
            break;
        case DAX_WORD:
        case DAX_UINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_uint = plc_tag_get_uint16(tag->tag_id, offset*tag->elem_size);
                ((dax_uint *)tag->buff)[offset] = val.dax_uint;
            }
            break;
        case DAX_INT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_int = plc_tag_get_int16(tag->tag_id, offset*tag->elem_size);
                ((dax_int *)tag->buff)[offset] = val.dax_int;
            }
            break;
        case DAX_DWORD:
        case DAX_UDINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_udint = plc_tag_get_uint32(tag->tag_id, offset*tag->elem_size);
                ((dax_udint *)tag->buff)[offset] = val.dax_udint;
            }
            break;
        case DAX_DINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_dint = plc_tag_get_int32(tag->tag_id, offset*tag->elem_size);
                ((dax_dint *)tag->buff)[offset] = val.dax_dint;
            }
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_ulint = plc_tag_get_uint64(tag->tag_id, offset*tag->elem_size);
                ((dax_ulint *)tag->buff)[offset] = val.dax_ulint;
            }
            break;
        case DAX_LINT:
        case DAX_TIME:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_lint = plc_tag_get_int64(tag->tag_id, offset*tag->elem_size);
                ((dax_lint *)tag->buff)[offset] = val.dax_lint;
            }
            break;
        case DAX_REAL:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_real = plc_tag_get_float32(tag->tag_id, offset*tag->elem_size);
                ((dax_real *)tag->buff)[offset] = val.dax_real;
            }
            break;
        case DAX_LREAL:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_lreal = plc_tag_get_float32(tag->tag_id, offset*tag->elem_size);
                ((dax_lreal *)tag->buff)[offset] = val.dax_lreal;
            }
            break;
        default: /* This should be a CDT */
            if(tag->byte_map != NULL) {
                for(offset = 0; offset < tag->h.count; offset++) {
                    int i = offset * tag->elem_size;
                    plc_tag_get_raw_bytes(tag->tag_id, i, rawbuff, tag->elem_size);
                    for(int n=0;n<tag->elem_size;n++) {
                        tag->buff[i+n] = rawbuff[tag->byte_map[n]];
                    }
                }
            } else {
                for(offset = 0; offset < tag->h.count; offset++) {
                    int i = offset * tag->elem_size;
                    plc_tag_get_raw_bytes(tag->tag_id, i, &tag->buff[offset * tag->h.size / tag->h.count], tag->elem_size);
                }
            }
            break;
    }
}


static void
_tag_callback(int32_t tag_id, int event, int status, void *udata) {
    struct tagdef *tag = (struct tagdef *)udata;

    /* handle the events. */
    switch(event) {
        case PLCTAG_EVENT_ABORTED:
            dax_log(DAX_LOG_COMM, "%s: Tag operation was aborted with status %s!", tag->daxtag, plc_tag_decode_error(status));
            break;

        case PLCTAG_EVENT_CREATED:
            if(tag->elem_size == -1) { /* -1 if not set yet */
                tag->elem_size = plc_tag_get_int_attribute(tag->tag_id, "elem_size", 0);
            }
            dax_log(DAX_LOG_COMM, "%s: Tag created with status %s - element size = %d.",
                                   tag->daxtag, plc_tag_decode_error(status), tag->elem_size);
            break;

        case PLCTAG_EVENT_DESTROYED:
            dax_log(DAX_LOG_COMM, "%s: Tag was destroyed with status %s.", tag->daxtag, plc_tag_decode_error(status));
            break;

        case PLCTAG_EVENT_READ_COMPLETED:
            _read_tag(tag);
            if(tag_queue_push(tag)) {
                dax_log(DAX_LOG_ERROR, "Tag read queue is full");
            }
            break;

        case PLCTAG_EVENT_READ_STARTED:
            //DF("%s: Tag read operation started with status %s.", tag->daxtag, plc_tag_decode_error(status));
            break;

        case PLCTAG_EVENT_WRITE_COMPLETED:
            //DF("%s: Tag write operation completed with status %s!", tag->daxtag, plc_tag_decode_error(status));
            break;

        case PLCTAG_EVENT_WRITE_STARTED:
            //DF("%s: Tag write operation started with status %s.", tag->daxtag, plc_tag_decode_error(status));
            break;

        default:
            DF("%s: Unexpected event %d!", tag->daxtag, event);
            break;
    }
    //DF("tag = %s, event = %d, status = %d", tag->daxtag, event, status);
}

static void
_create_plctags(void) {
    struct tagdef *this;

    this = taglist;
    while(this != NULL) {
        this->tag_id = plc_tag_create_ex(this->plctag, _tag_callback, this, 0);
        if(this->tag_id < 0) {
            dax_log(DAX_LOG_ERROR, "Unable to create tag %s", this->plctag);
        } else {
            if(this->read_update > 0) {
                plc_tag_set_int_attribute(this->tag_id, "auto_sync_read_ms", this->read_update);
            }
            if(this->write_update > 0) {
                plc_tag_set_int_attribute(this->tag_id, "auto_sync_write_ms", this->write_update);
            }
        }
        this = this->next;
    }
}


/* This is the callback that gets called when tags that we are going to write to the PLC are
   changed in the tag server.*/
static void
_event_callback(dax_state *ds, void *udata) {
    struct tagdef *tag = (struct tagdef *)udata;
    int offset;
    dax_type_union val;
    uint8_t rawbuff[tag->elem_size];

    dax_event_get_data(ds, tag->buff, tag->h.size);
    switch(tag->h.type) {
        case DAX_BOOL:
            for(offset = 0; offset < tag->h.count; offset++) {
                if(tag->buff[offset/8] & 0x01 << (offset % 8)) {
                    plc_tag_set_bit(tag->tag_id, offset, 1);
                } else {
                    plc_tag_set_bit(tag->tag_id, offset, 0);
                }
            }
            break;
        case DAX_BYTE:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_byte = ((dax_byte *)tag->buff)[offset];
                plc_tag_set_uint8(tag->tag_id, offset*tag->elem_size, val.dax_byte);
            }
            break;
        case DAX_SINT:
        case DAX_CHAR:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_sint = ((dax_sint *)tag->buff)[offset];
                plc_tag_set_int8(tag->tag_id, offset*tag->elem_size, val.dax_sint);
            }
            break;
        case DAX_WORD:
        case DAX_UINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_uint = ((dax_uint *)tag->buff)[offset];
                plc_tag_set_uint16(tag->tag_id, offset*tag->elem_size, val.dax_uint);
            }
            break;
        case DAX_INT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_int = ((dax_int *)tag->buff)[offset];
                plc_tag_set_int16(tag->tag_id, offset*tag->elem_size, val.dax_int);
            }
            break;
        case DAX_DWORD:
        case DAX_UDINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_udint = ((dax_udint *)tag->buff)[offset];
                plc_tag_set_uint32(tag->tag_id, offset*tag->elem_size, val.dax_udint);
            }
            break;
        case DAX_DINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_dint = ((dax_dint *)tag->buff)[offset];
                plc_tag_set_int32(tag->tag_id, offset*tag->elem_size, val.dax_dint);
            }
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_ulint = ((dax_ulint *)tag->buff)[offset];
                plc_tag_set_uint64(tag->tag_id, offset*tag->elem_size, val.dax_ulint);
            }
            break;
        case DAX_LINT:
        case DAX_TIME:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_lint = ((dax_lint *)tag->buff)[offset];
                plc_tag_set_int64(tag->tag_id, offset*tag->elem_size, val.dax_lint);
            }
            break;
        case DAX_REAL:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_real = ((dax_real *)tag->buff)[offset];
                plc_tag_set_float32(tag->tag_id, offset*tag->elem_size, val.dax_real);
            }
            break;
        case DAX_LREAL:
            for(offset = 0; offset < tag->h.count; offset++) {
                val.dax_lreal = ((dax_lreal *)tag->buff)[offset];
                plc_tag_set_float64(tag->tag_id, offset*tag->elem_size, val.dax_lreal);
            }
            break;
        default:
            if(tag->byte_map != NULL) {
                for(offset = 0; offset < tag->h.count; offset++) {
                    int i = offset * tag->elem_size;
                    for(int n=0;n<tag->elem_size;n++) {
                        rawbuff[tag->byte_map[n]] = tag->buff[i+n];
                    }
                    plc_tag_set_raw_bytes(tag->tag_id, i, rawbuff, tag->elem_size);
                }
            } else {
                /* Any other type will just be a raw transfer */
                for(offset = 0; offset < tag->h.count; offset++) {
                    int i = offset * tag->elem_size;
                    plc_tag_set_raw_bytes(tag->tag_id, i, &tag->buff[offset * tag->h.size / tag->h.count], tag->elem_size);
                }
            }
            break;
    }
}

static void
_check_tag_status(void) {
    struct tagdef *this;
    int result;
    dax_id id;

    this = taglist;
    while(this != NULL) {
        /* If this is NULL then we haven't found the tag in the tagserver yet */
        if(this->buff == NULL) {
            result = dax_tag_handle(ds, &this->h, this->daxtag, this->count);
            if(result == ERR_OK) {
                this->buff = malloc(this->h.size);
                if(this->write_update > 0) {
                    result = dax_event_add(ds, &this->h, EVENT_CHANGE, NULL, &id, _event_callback, this, NULL);
                    if(result) {
                        dax_log(DAX_LOG_ERROR, "Unable to add change event to tag %s - %s", this->daxtag, dax_errstr(result));
                    } else {
                        dax_event_options(ds, id, EVENT_OPT_SEND_DATA);
                    }
                }
            } else {
                dax_log(DAX_LOG_ERROR, "Unable to find tag %s - %s", this->daxtag, dax_errstr(result));
            }
        }
        this = this->next;
    }
}


static void
_log_callback(int32_t tag_id, int debug_level, char *message) {
    message[strlen(message)-1] = '\0'; /* Remove the newline that is passed */
    if(debug_level == PLCTAG_DEBUG_ERROR) {
        dax_log(DAX_LOG_ERROR, "tag:%d - %s", tag_id, message);
    } else if(debug_level == PLCTAG_DEBUG_WARN) {
        dax_log(DAX_LOG_WARN, "tag:%d - %s", tag_id, message);
    } else {
        dax_log(DAX_LOG_DEBUG, "tag:%d - %s", tag_id, message);
    }
}

/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;
    int result = 0;

    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    if(plc_tag_check_lib_version(2,5,0) != PLCTAG_STATUS_OK) {
        dax_log(DAX_LOG_FATAL, "Required compatible plctag library version %d.%d.%d not available!", 2,5,0);
        exit(-1);
    }

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("plctag"); /* Replace 'skel' with your module name */
    if(ds == NULL) {
        dax_log(DAX_LOG_FATAL, "Unable to Allocate DaxState Object\n");
    }

    /* Execute the configuration */
    result = plctag_configure(argc, argv);
    if(result) exit(-1);

    dax_free_config (ds);

    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_log(DAX_LOG_FATAL, "Unable to find OpenDAX");
        exit(-1);
    }

    _runsignal = 1;
    result = dax_set_run_callback(ds, _run_function);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to add run callback - %s", dax_errstr(result));
    }
    result = dax_set_stop_callback(ds, _stop_function);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to add stop callback - %s", dax_errstr(result));
    }
    result = dax_set_kill_callback(ds, _kill_function);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to add kill callback - %s", dax_errstr(result));
    }
    dax_set_status(ds, "OK");

    dax_log(DAX_LOG_MINOR, "PLCTAG Module Starting");
    plc_tag_set_debug_level(PLCTAG_DEBUG_WARN);
    plc_tag_register_logger(_log_callback);
    tag_thread_start();
    _create_plctags();
    _check_tag_status();
    dax_set_running(ds, 1);
    while(1) {
    	/* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_log(DAX_LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        dax_event_wait(ds, 1000, NULL);
        _check_tag_status();
    }

 /* This is just to make the compiler happy */
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
