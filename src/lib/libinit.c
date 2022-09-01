/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2009 Phil Birkelbach
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
 *  Source code file for the library initialization functions.
 */


#include <libdax.h>
#include <libcommon.h>

/*!
 *  Allocate and initialize the state of the dax_state connection
 *  object.  The returned object will need to be passed to
 *  dax_free() when finished.
 * 
 *  @param name Pointer to a string that represents the name that will
 *              identify our module to the rest of the sytem.
 * 
 *  @return An opaque pointer that represents the Dax State Object.  This
 *          pointer is sent to the vast majority of the functiosn in this
 *          library.  It represents everything about the connection to the
 *          tag server.
 */
dax_state *
dax_init(char *name)
{
    dax_state *ds;
    ds = malloc(sizeof(dax_state));
    if(ds == NULL) return NULL;

    dax_init_logger(name, 0);

    ds->attr_head = NULL;
    ds->L = NULL;
    ds->modulename = strdup(name);
    if(ds->modulename == NULL) return NULL;
    
    ds->msgtimeout = 0;
    ds->sfd = -1;       /* Server's File Descriptor */
    ds->reformat = 0;  /* Flags to show how to reformat the incoming data */
    ds->logflags = 0;
    /* Tag Cache */
    ds->cache_head = NULL;     /* First node in the cache list */
    ds->cache_limit = 0;       /* Total number of nodes that we'll allocate */
    ds->cache_count = 0;       /* How many nodes we actually have */
    /* datatype list */
    ds->datatypes = NULL;
    ds->datatype_size = 0;
    /* Event list array */
    ds->events = malloc(sizeof(event_db));
    if(ds->events == NULL) {
        free(ds->modulename);
        free(ds);
        return NULL;
    }
    ds->last_msg = NULL;
    ds->event_size = 1;
    ds->event_count = 0;
    /* Event Message FIFO Queue */
    ds->emsg_queue = malloc(sizeof(dax_message *)*EVENT_QUEUE_SIZE);
    ds->emsg_queue_size = EVENT_QUEUE_SIZE;     /* Total size of the Event Message Queue */
    ds->emsg_queue_count = 0;    /* number of entries in the event message queue */
    /* Initialize locks and condition variables */
    pthread_mutex_init(&ds->lock, NULL);
    pthread_mutex_init(&ds->event_lock, NULL);
    pthread_mutex_init(&ds->msg_lock, NULL);
    pthread_cond_init(&ds->event_cond, NULL);
    pthread_cond_init(&ds->msg_cond, NULL);

    return ds;
}


/*!
 * Deallocate and free the given dax_state object
 * 
 * @param ds The Dax State object that is to be deallocated
 *           and freed.  This would be the same pointer that
 *           would have been returned from the dax_init() function.
 */
int
dax_free(dax_state *ds)
{
    pthread_mutex_unlock(&ds->lock);
    pthread_mutex_destroy(&ds->lock);
    free(ds->modulename);
    /* TODO: gotta loop through and free the udata in the events. */
    free(ds->events);
    free(ds->emsg_queue);
    free(ds);
    return 0;
}
