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

/* TODO: there is probably a better way than copying the memory */
int
push_event(dax_state *ds, dax_message *msg) {
	int next, newsize;
	dax_message *new_queue;

    /* We need to grow the queue */
    if(ds->emsg_queue_count == ds->emsg_queue_size) {
		newsize = ds->emsg_queue_size * 2;
        new_queue = realloc(ds->emsg_queue, sizeof(dax_message) * newsize);
        if(new_queue == NULL) return ERR_ALLOC;
        ds->emsg_queue = new_queue;
        /* After the queue size is increased we need to move the fragment that is now
         * at the beginning of the queue to the start of the new area */
        if(ds->emsg_queue_read > 0) {
            memcpy(&ds->emsg_queue[ds->emsg_queue_size],&ds->emsg_queue[0], ds->emsg_queue_last+1);
            ds->emsg_queue_last = ds->emsg_queue_size;
        }
        ds->emsg_queue_size = newsize;
	}
	next = ds->emsg_queue_last;
	memcpy(&ds->emsg_queue[next], msg, sizeof(dax_message));
	ds->emsg_queue_last++;
	/* Roll over the last entry to the beginning of the queue */
	if(ds->emsg_queue_last == ds->emsg_queue_size) {
		ds->emsg_queue_last = 0;
	}
	ds->emsg_queue_count++;

	return 0;
}

dax_message *
pop_event(dax_state *ds) {
    dax_message *temp;

    if(ds->emsg_queue_count == 0) {
        return NULL;
    }
    temp = &ds->emsg_queue[ds->emsg_queue_read];
    ds->emsg_queue_read++;
    if(ds->emsg_queue_read == ds->emsg_queue_size)  {
        ds->emsg_queue_read = 0;
    }
    ds->emsg_queue_count--;

    return temp;
}

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
            memmove(&ds->events[n], &ds->events[n+1], sizeof(event_db) * (ds->event_count - n-1));
            if(ds->events[n].free_callback) {
                ds->events[n].free_callback(ds->events[n].udata);
            }
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
dispatch_event(dax_state *ds, dax_message msg, dax_id *id)
{
    int n;
    u_int32_t idx, eid;

    idx =      ntohl(*(u_int32_t *)(&msg.data[0]));
    eid =      ntohl(*(u_int32_t *)(&msg.data[4]));
    /* we just store the pointer to the message data in case the callback needs it
     * This data can be retrieved in the callback by dax_event_get_dat() */
    ds->event_data = &msg.data[8];
    ds->event_data_size = msg.size-8;
	for(n = 0; n < ds->event_count; n ++) {
		if(ds->events[n].idx == idx && ds->events[n].id == eid) {
			if(ds->events[n].callback != NULL) {
				ds->events[n].callback(ds, ds->events[n].udata);
			}
			if(id != NULL) {
				id->id = eid;
				id->index = idx;
			}
			return 0;
		}
	}
	ds->event_data = NULL; /* This indicates that the data is out of scope now */
	dax_error(ds, "dax_event_dispatch() received an event that does not exist in database");
	return ERR_GENERIC;
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
    struct timeval tval;
    fd_set fds;
    int done = 0;
    dax_message msg;
    dax_message *msg_ptr;

    /* Check the event message queue and dispatch that event if
         * there is one there.*/
    msg_ptr = pop_event(ds);
    if(msg_ptr != NULL) {
        return dispatch_event(ds, *msg_ptr, id);
    }
    /* ...otherwise wait on the socket */
    while(!done) {
        FD_ZERO(&fds);
        FD_SET(ds->sfd, &fds);
        tval.tv_sec = timeout / 1000;
        tval.tv_usec = (timeout % 1000) * 1000;
        if(timeout > 0) {
            result = select(ds->sfd + 1, &fds, NULL, NULL, &tval);
        } else {
            result = select(ds->sfd + 1, &fds, NULL, NULL, NULL);
        }
        if(result > 0) {
            result = message_get(ds->sfd, &msg);
            if(result) return result;
            result = dispatch_event(ds, msg, id);
            if(result == 0) done = 1;
        } else if (result < 0) {
            return result;
        } else {
            return ERR_TIMEOUT;
        }
    }
    return 0;
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
    int result;
    struct timeval tval;
    fd_set fds;
    dax_message msg;
    dax_message *msg_ptr;

    /* Check the event message queue and dispatch that event if
         * there is one there.*/
    msg_ptr = pop_event(ds);
    if(msg_ptr != NULL) {
        return dispatch_event(ds, *msg_ptr, id);
    }
    /*... otherwise check the socket */
    FD_ZERO(&fds);
    FD_SET(ds->sfd, &fds);
    tval.tv_sec = 0;
    tval.tv_usec = 0;
    result = select(ds->sfd + 1, &fds, NULL, NULL, &tval);
    if(result > 0) {
        result = message_get(ds->sfd, &msg);
        if(result) return result;
        return dispatch_event(ds, msg, id);
    } else if (result < 0) {
        return ERR_GENERIC;
    } else {
        return ERR_NOTFOUND;
    }
    return 0;
}

/*!
 * This function will return the asynchronous event handling file
 * descriptor to the module.  This is used if the module wants to handle
 * it's own file descriptor management.  Handy for event driven programs
 * that need to select() on multiple file descriptors
 */
int
dax_event_get_fd(dax_state *ds)
{
    return ds->sfd;
}


/*!
 * Retrieves the data that was passed back from the server when the event fired.
 * This function is designed to be called from inside the callback function that
 * is called when the event is fired.  This data will go out of scope after the
 * event callback function returns.  If the data is meant to persist it will be
 * the responsibility of the event callback function to store it.
 *
 * If this function is called while the data is out of scope and error will be
 * returned.
 *
 * @param ds   Pointer to the dax state object
 * @param buff Pointer to the buffer where the event data is to be written
 * @param len  Pointer to the length of the buffer that is passed.  This will
 *             be changed by the function if
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
