/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *

 * This file contains the event database and dispatching functions
 */

#include <libdax.h>
#include <common.h>
#include <arpa/inet.h>

/*!
 * Convert the given string to a dax event type
 * that is suitable for passing to the event
 * handling function.
 *
 * @param string The string to convert
 * @returns the type or 0 for error
 * */
int
dax_event_string_to_type(char *string) {
    if(!strcasecmp(string, "WRITE")) {
        return EVENT_WRITE;
    } else if(!strcasecmp(string, "CHANGE")) {
        return EVENT_CHANGE;
    } else if(!strcasecmp(string, "SET")) {
        return EVENT_SET;
    } else if(!strcasecmp(string, "RESET")) {
        return EVENT_RESET;
    } else if(!strcasecmp(string, "EQUAL")) {
        return EVENT_EQUAL;
    } else if(!strcasecmp(string, "GREATER")) {
        return EVENT_GREATER;
    } else if(!strcasecmp(string, "LESS")) {
        return EVENT_LESS;
    } else if(!strcasecmp(string, "DEADBAND")) {
        return EVENT_DEADBAND;
    } else {
        return 0;
    }
}

/*!
 * Covert the given type into it's string representation.
 *
 * @param type The dax data type
 * @returns A pointer to the string representing the event type or NULL on error
 */
const char *
dax_event_type_to_string(int type) {
    switch(type) {
        case EVENT_WRITE:
            return "WRITE";
        case EVENT_CHANGE:
            return "CHANGE";
        case EVENT_SET:
            return "SET";
        case EVENT_RESET:
            return "RESET";
        case EVENT_EQUAL:
            return "EQUAL";
        case EVENT_GREATER:
            return "GREATER";
        case EVENT_LESS:
            return "LESS";
        case EVENT_DEADBAND:
            return "DEADBAND";
        default:
            return NULL;
    }
}

/* Store the event information into a database internal to the library.  This is
 * where the callbacks and the userdata are stored.  The server simply sends an ID
 */
int
add_event(dax_state *ds, dax_id id, void *udata, void (*callback)(dax_state *ds, void *udata),
          void (*free_callback)(void *udata))
{
    event_db *new_db;

    /* check to see if we need to grow the database */
    if(ds->event_count == ds->event_size) {
        /* For now we just double the size */
        new_db = realloc(ds->events, (ds->event_size * 2) * sizeof(event_db));
        if(new_db != NULL) {
            ds->events = new_db;
            ds->event_size *= 2;
        } else {
            return ERR_ALLOC;
        }
    }
    /* For now we are just putting these in unsorted and the search
     * routine in the dispatch function will be a simple linear search */
    /* TODO: Sort this array and add binary search to event_dispatch() */
    ds->events[ds->event_count].idx = id.index;
    ds->events[ds->event_count].id = id.id;
    ds->events[ds->event_count].udata = udata;
    ds->events[ds->event_count].callback = callback;
    ds->events[ds->event_count].free_callback = free_callback;

    ds->event_count++;
    return 0;
}

/* Finds the event in the list and removes it.  It also calls the free_callback()
 * function if it is assigned */
int
del_event(dax_state *ds, dax_id id)
{
    int n;

    for(n = 0; n < ds->event_count; n++) {
        if(ds->events[n].idx == id.index && ds->events[n].id == id.id) {
            if(ds->events[n].free_callback) {
                ds->events[n].free_callback(ds->events[n].udata);
            }
            memmove(&ds->events[n], &ds->events[n+1], sizeof(event_db) * (ds->event_count - n-1));
            ds->event_count--;
            return 0;
        }
    }
    return ERR_NOTFOUND;
}

/* This function deals with a single event.
 *
 * @param ds Pointer to the dax state object
 * @param msg the message that we are going to react too
 * @param id Pointer to an event id that will be filled in by this function
 *           with the information of the event that was handled.  If set to
 *           NULL the function will do nothing with this pointer.
 * @returns zero on success or an error code otherwise
 */
int
dispatch_event(dax_state *ds, dax_message *msg, dax_id *id)
{
    int n;
    uint32_t idx, eid;

    idx =      ntohl(*(uint32_t *)(&msg->data[0]));
    eid =      ntohl(*(uint32_t *)(&msg->data[4]));
    /* we just store the pointer to the message data in case the callback needs it
     * This data can be retrieved in the callback by dax_event_get_data() */
    ds->event_data = &msg->data[8];
    ds->event_data_size = msg->size-8;
    for(n = 0; n < ds->event_count; n ++) {
        if(ds->events[n].idx == idx && ds->events[n].id == eid) {
            if(ds->events[n].callback != NULL) {
                ds->events[n].callback(ds, ds->events[n].udata);
            }
            if(id != NULL) {
                id->id = eid;
                id->index = idx;
            }
            ds->event_data = NULL; /* This indicates that the data is out of scope now */
            return 0;
        }
    }
    ds->event_data = NULL; /* This indicates that the data is out of scope now */
    dax_log(DAX_LOG_ERROR, "dax_event_dispatch() received an event that does not exist in database");
    return ERR_GENERIC;
}

static void
_pop_event(dax_state *ds, dax_message *msg) {
    int n;

    msg->msg_type = ds->emsg_queue[0]->msg_type;
    msg->size = ds->emsg_queue[0]->size;
    msg->fd = ds->emsg_queue[0]->fd;
    memcpy(msg->data, ds->emsg_queue[0]->data, ds->emsg_queue[0]->size);
    free(ds->emsg_queue[0]); /* Free the top one */
    /* Move the rest up */
    for(n = 0;n<ds->emsg_queue_count-1;n++) {
        ds->emsg_queue[n] = ds->emsg_queue[n+1];
    }
    ds->emsg_queue_count--;
}
/*!
 * Blocks waiting for an event to happen.  If an event is found it
 * will run the callback function for that event.
 * @param ds Pointer to the dax state object
 * @param timeout Number of milliseconds to wait for an event.  If
 *                set to zero it will wait forever.
 * @param id Pointer to an event id structure.  The function will
 *           populate this structure with the id of the event that
 *           was handled.  Can be set to NULL if unneeded.
 * @returns ERR_TIMEOUT if the time expires, zero on success or other
 *          error codes if appropriate.
 */
int
dax_event_wait(dax_state *ds, int timeout, dax_id *id)
{
    int result;
    struct timespec ts;
    dax_message msg;

    pthread_mutex_lock(&ds->event_lock);
    while(ds->emsg_queue_count == 0) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout/1000;
        ts.tv_nsec += timeout%1000 * 1e6;
        if(ts.tv_nsec > 1e9) {
            ts.tv_sec++;
            ts.tv_nsec -= 1e9;
        }
        if(timeout) {
            result = pthread_cond_timedwait(&ds->event_cond, &ds->event_lock, &ts);
        } else {
            result = pthread_cond_wait(&ds->event_cond, &ds->event_lock);
        }
        if(result == ETIMEDOUT) {
            pthread_mutex_unlock(&ds->event_lock);
            return ERR_TIMEOUT;
        }
        assert(result == 0);

    }
    /* We should have a message once we get here */
    /* Pop the message off of the event queue before we dispatch
     * the event so that we can turn control back over to the connection
     * thread. */
    _pop_event(ds, &msg);
    pthread_mutex_unlock(&ds->event_lock);
    return dispatch_event(ds, &msg, id);
}

/*!
 * Checks for a pending event without blocking.  If there is an
 * event pending it will run the callback for that event.
 *
 * @param ds Pointer to the dax state object
 * @param id Pointer to an event id structure.  The function will
 *           populate this structure with the id of the event that
 *           was handled.  Can be set to NULL if unneeded.
 * @returns ERR_NOTFOUND if there are no pending events, zero
 *          if an event was successfully dispatched or other
 *          error codes if appropriate.
 */
int
dax_event_poll(dax_state *ds, dax_id *id)
{
    dax_message msg;
    pthread_mutex_lock(&ds->event_lock);
    if(ds->emsg_queue_count > 0) {
        _pop_event(ds, &msg);
        pthread_mutex_unlock(&ds->event_lock);
        return dispatch_event(ds, &msg, id);
    }
    pthread_mutex_unlock(&ds->event_lock);
    return ERR_NOTFOUND;
}

/*!
 * Retrieves the data that was passed back from the server when the event fired.
 * This function is designed to be called from inside the callback function that
 * is called when the event is fired.  This data will go out of scope after the
 * event callback function returns.  If the data is meant to persist it will be
 * the responsibility of the event callback function to store it.
 *
 * The option EVENT_OPT_SEND_DATA will have to be set for the event with the
 * dax_event_options() function or no data will be returned by the server.
 *
 * If this function is called while the data is out of scope an error will be
 * returned.
 *
 * @param ds   Pointer to the dax state object
 * @param buff Pointer to the buffer where the event data is to be written
 * @param len  Maximum size of buff that can be written
 * @returns    number of bytes written on success or a negative error code otherwise.
 */
int
dax_event_get_data(dax_state *ds, void* buff, int len) {
    int size;

    size = MIN(len, ds->event_data_size);
    if(ds->event_data == NULL) return ERR_DELETED;
    if(ds->event_data_size == 0) return ERR_EMPTY;

    memcpy(buff, ds->event_data, size);
    return size;
}
