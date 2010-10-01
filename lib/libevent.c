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

 * This file contains the event database and dispatching functions 
 */

#include <libdax.h>
#include <common.h>

int
add_event(dax_state *ds, dax_event_id id, void *udata, void (*callback)(void *udata)) {
    event_db *new_db;

    /* TODO: Should find holes in the array from previous deletions and add new ones there. */
    
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
    
    ds->event_count++;
    return 0;
}

int
del_event(dax_state *ds, dax_event_id id) {
    int n;

    for(n = 0; n < ds->event_count; n++) {
        if(ds->events[n].idx == id.index && ds->events[n].id == id.id) {
            memmove(&ds->events[n], &ds->events[n+1], sizeof(event_db) * (ds->event_count - n-1));
            ds->event_count--;
            return 0;
        }
    }
    return ERR_NOTFOUND;
}


/* Blocks waiting for an event to happen.  If an event is found it
 * will run the callback function for that event.  Returns ERR_TIMEOUT 
 * if the time expires.  Returns zero on success. */
int
dax_event_select(dax_state *ds, int timeout, dax_event_id *id) {
    int result;
    struct timeval tval;
    fd_set fds;
    int done = 0;
    
    while(!done) {
        FD_ZERO(&fds);
        FD_SET(ds->afd, &fds);
        tval.tv_sec = timeout / 1000;
        tval.tv_usec = timeout % 1000;
        result = select(ds->afd + 1, &fds, NULL, NULL, &tval);
        if(result > 0) {
            result = dax_event_dispatch(ds, id);
            if(result == 0) done = 1;
        } else if (result < 0) {
            return ERR_GENERIC;
        } else {
            return ERR_TIMEOUT;
        }
    }
    return 0;
}

/* Does not block and checks for a pending event.  If there is an
 * event pending it will run the callback for that event.  Returns
 * 0 if it services an event and ERR_NOTFOUND if there are no
 * events pending to service */
int
dax_event_poll(dax_state *ds, dax_event_id *id) {
    int result;
    struct timeval tval;
    fd_set fds;
    
    FD_ZERO(&fds);
    FD_SET(ds->afd, &fds);
    tval.tv_sec = 0;
    tval.tv_usec = 0;
    result = select(ds->afd + 1, &fds, NULL, NULL, &tval);
    if(result > 0) {
        return dax_event_dispatch(ds, id);
    } else if (result < 0) {
        return ERR_GENERIC;
    } else {
        return ERR_NOTFOUND;
    }
    return 0;
}

/* This function will return the asynchronous event handling file
 * descriptor to the module.  This is used if the module wants to handle
 * it's own file descriptor management.  Handy for event driven programs
 * that need to select() on multiple file descriptors */
int
dax_event_get_fd(dax_state *ds) {
    return ds->afd;
}

/* This calls the read() system call to get the event that SHOULD be pending
 * on the ds->afd file descriptor, then it calls the event callback if it
 * is necessary and returns the id through the pointer.  This is used from
 * both the dax_event_select() and dax_event_poll() library function calls and
 * can be called from modules that choose to take care of dealing with the
 * file descriptor themselves.  It returns 0 if it was able to dispatch
 * the event, ERR_NOTFOUND if it only received a partial event message from
 * the server and other errors if necessary. */
int
dax_event_dispatch(dax_state *ds, dax_event_id *id) {
    int result, n;
    u_int32_t etype, idx, eid, byte, count, datatype;
    u_int8_t bit;
    
//    fprintf(stderr, "dax_event_dispatch() called\n");
    result = read(ds->afd, &(ds->ebuff[ds->eindex]), EVENT_MSGSIZE - ds->eindex);
    ds->eindex += result;
//    fprintf(stderr, "ds->eindex = %d\n", ds->eindex);
    if(ds->eindex == EVENT_MSGSIZE) { /* We have a full message now */
//        fprintf(stderr, "dax_event_dispatch() firing event\n");
        etype =    ntohl(*(u_int32_t *)(&ds->ebuff[0]));
        idx =      ntohl(*(u_int32_t *)(&ds->ebuff[4]));
        eid =      ntohl(*(u_int32_t *)(&ds->ebuff[8]));
        byte =     ntohl(*(u_int32_t *)(&ds->ebuff[12]));
        count =    ntohl(*(u_int32_t *)(&ds->ebuff[16]));
        datatype = ntohl(*(u_int32_t *)(&ds->ebuff[20]));
        bit =      *(u_int8_t *)(&ds->ebuff[24]);
//        fprintf(stderr, "event type  = %d\n", etype);
//        fprintf(stderr, "event idx   = %d\n", idx);
//        fprintf(stderr, "event id    = %d\n", eid);
//        fprintf(stderr, "event byte  = %d\n", byte);
//        fprintf(stderr, "event bit   = %d\n", bit);
//        fprintf(stderr, "event count = %d\n", count);
//        fprintf(stderr, "event datatype = %s\n", dax_type_to_string(ds, datatype));
        ds->eindex = 0; /* Reset for the next time */
        for(n = 0; n < ds->event_count; n ++) {
//            fprintf(stderr, "checking for event at n = %d, idx = %d, id = %d\n", n, ds->events[n].idx, ds->events[n].id );
            if(ds->events[n].idx == idx && ds->events[n].id == eid) {
                if(ds->events[n].callback != NULL) {
                    ds->events[n].callback(ds->events[n].udata);
                }
                if(id != NULL) {
                    id->id = eid;
                    id->index = idx;
                }
                return 0;
            }
        }
        dax_error(ds, "dax_event_dispatch() recieved an event that does not exist in database");
        return ERR_GENERIC;
    } else {
        return ERR_NOTFOUND;
    }
    return 0;
}
