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

 * Source file for the worker threads
 */

#include "daxlua.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>

extern dax_state *ds;

/* script queue threads threads
   These threads are basically worker threads that will take
   scrfipts off of the script queue and run them. */
static unsigned int q_threadcount;
static unsigned int queue_size;
static pthread_t *q_threads;
static int *q_thread_ids;
static int q_overruns;

static pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t q_cond = PTHREAD_COND_INITIALIZER;
static pthread_attr_t attr;

static script_t **script_queue;
static int q_first, q_last;
static int q_full, q_empty;
static int q_exit;


/* interval threads
   These threads are designed to be run on a periodic basis.  They
   run every script in their list each time the interval elapses.
   This is a linked list. */
static interval_thread_t *i_threads = NULL;

void
thread_set_qthreadcount(unsigned int count) {
    q_threadcount = count;
}

void
thread_set_queue_size(unsigned int size) {
    queue_size = size;
}

/*** Queue Handling Functions ***/
/* Pushes a script pointer onto the queue array
   The q_lock mutex must be held before calling this function
   returns -1 if the queue is full */
static int
_q_push(script_t *script) {
    if(q_full) return -1;
    //DF("pushing %s, q_first = %d, q_last = %d",script->name, q_first, q_last);
    script_queue[q_last] = script;
    q_last++;
    if(q_last == q_first) q_full = 1;
    q_empty = 0;
    if(q_last >= queue_size) q_last = 0;
    return 0;
}

/* Pushes a script pointer onto the queue array
   The q_lock mutex must be held before calling this function
   returns NULL if the queue is empty */
static script_t *
_q_pop(void) {
    script_t *x;
    if(q_empty) return NULL;
    x = script_queue[q_first];
    //DF("poping %s, q_first = %d, q_last = %d",((script_t *)x)->name, q_first, q_last);
    q_first++;
    if(q_first >= queue_size) q_first = 0;
    if(q_first == q_last) q_empty = 1;
    q_full = 0;
    return x;
}


/* add an interval thread to the linked list.  We'll start them later */
int
add_interval_thread(char *name, long interval) {
    interval_thread_t *new_thread, *this;

    new_thread = malloc(sizeof(interval_thread_t));
    if(new_thread == NULL) {
        dax_log(LOG_FATAL, "Unable to allocate memory for new interval thread %s", name);
        return ERR_ALLOC;
    }

    new_thread->name = strdup(name);
    new_thread->interval = interval;
    new_thread->overruns = 0;
    new_thread->scriptcount = 0;
    new_thread->scripts = NULL;
    new_thread->next = NULL;

    if(i_threads == NULL) {
        i_threads = new_thread;
    } else {
        this = i_threads;
        while(this->next != NULL) {
            this = this->next;
        }
        this->next = new_thread;
    }

    return 0;
}

static void *
_interval_thread(void* thread_def) {
    interval_thread_t *t;
    struct timespec start, end;
    long long sleep_time;

    t = (interval_thread_t *)thread_def;

    while(1) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(int n=0;n<t->scriptcount;n++) {
            run_script(t->scripts[n]);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        sleep_time = (t->interval * 1000) - ((end.tv_sec - start.tv_sec) * 1e6) + ((end.tv_nsec - start.tv_nsec) / 1e3);
        if(sleep_time < 0) {
            t->overruns++;
            *(dax_udint *)(t->tag_data + DAX_TAGNAME_SIZE+ sizeof(dax_udint)*2) = t->overruns;
            dax_tag_write(ds, t->handle, t->tag_data);
        } else {
            usleep(sleep_time); /* Just for now need to calculate the time */
        }

    }

    return NULL;
}


static int
_start_interval_thread(interval_thread_t *t) {
    int count = 0;
    int n, x;
    script_t *s;
    pthread_attr_t attr;

    /* We start by counting the scripts that we need */
    t->scriptcount = 0;
    count = get_script_count();
    for(n=0;n<count;n++) {
        s = get_script(n);
        if(s->threadname != NULL && strcmp(t->name, s->threadname) == 0) {
            t->scriptcount++;
        }
    }

    t->tag_data = malloc(t->handle.size);
    if(t->tag_data == NULL) {
        dax_log(LOG_ERROR, "Unable to allocate memory for thread tag");
    } else {
        bzero(t->tag_data, t->handle.size);
        strncpy(t->tag_data, t->name, DAX_TAGNAME_SIZE);
        *(dax_udint *)(t->tag_data + DAX_TAGNAME_SIZE) = t->interval;
        *(dax_udint *)(t->tag_data + DAX_TAGNAME_SIZE+ sizeof(dax_udint)) = t->scriptcount;
        *(dax_udint *)(t->tag_data + DAX_TAGNAME_SIZE+ sizeof(dax_udint)*2) = t->overruns;
        dax_tag_write(ds, t->handle, t->tag_data);
    }

    if(t->scriptcount == 0) {
        dax_log(LOG_WARN, "Interval thread '%s' has no scripts assigned.  Not starting.", t->name);
        return ERR_GENERIC;
    }
    /* Now we allocate our array */
    t->scripts = malloc(sizeof(script_t) * t->scriptcount);
    if(t->scripts == NULL) {
        dax_log(LOG_ERROR, "Unable to allocate script array for thread %s", t->name);
        return ERR_ALLOC;
    }
    /* No we go through and assign the scripts to the array */
    x = 0;
    for(n=0;n<count;n++) {
        s = get_script(n);
        if(s->threadname != NULL && strcmp(t->name, s->threadname) == 0) {
            t->scripts[x++] = s;
            s->interval = t->interval;
        }
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if(pthread_create(&t->thread, &attr, (void *)&_interval_thread, (void *)t)) {
        dax_log(LOG_ERROR, "Unable to start interval thread - %s", t->name);
        return -1;
    } else {
        dax_log(LOG_MAJOR, "Started Interval Thread - %s", t->name);
        return 0;
    }

    return 0;
}

static void *
__queue_thread(void *num) {
    script_t *script;

    pthread_mutex_lock(&q_lock);
    while(1) {
        if(q_exit) break;
        pthread_cond_wait(&q_cond, &q_lock);

        while(! q_empty) {
            script = _q_pop(); // Get the job off of the queue <<sorta>>
            pthread_mutex_unlock(&q_lock); // play nice
            run_script(script);
            pthread_mutex_lock(&q_lock); // get it back
        }
    }
    pthread_mutex_unlock(&q_lock); // Give it up when we are done

    return NULL;
}


int
_start_queue_threads(void) {
    int n;

    q_threads = malloc(sizeof(pthread_t) * q_threadcount);
    q_thread_ids = malloc(sizeof(int) * q_threadcount);

    pthread_attr_init(&attr);

    for(n=0;n<q_threadcount;n++) {
        q_thread_ids[n] = n;
        pthread_create(&q_threads[n], NULL, __queue_thread, &q_thread_ids[n]);
    }
    return 0;
}

/* This function loops through all of the scripts that have
   triggers set and initializes them and then pushes them onto
   the queue so that the worker thread will run them. */
void
_run_queue_scripts(void) {
    int count;
    int n = 0;
    script_t *script;

    count = get_script_count();

    while(n<count) {
        script = get_script(n);

        if(script->script_trigger != NULL && (script->flags & CONFIG_AUTO_RUN)) {
            pthread_mutex_lock(&q_lock);
            if( ! q_full) {
                _q_push(script);
                pthread_cond_signal(&q_cond);
                pthread_mutex_unlock(&q_lock);
                n++;
            } else { /* If the queue is full */
                usleep(50000);
            }
        } else { /* Not an event script */
            n++;
        }
    }
}

/* This is the function that actually gets assigned to the opendax event.  The
   userdata pointer should be a pointer to the script that we wish to run */
static void
_script_event_dispatch(dax_state *d, void *udata) {
    script_t *script;
    trigger_t *t;
    int result;

    script = (script_t *)udata;
    t = script->script_trigger; /* Just to make life easier */

    if(t->buff != NULL) {
        /* We have to retrieve the data here but we cannot write it to the Lua
           state yet because of race conditions. */
        result = dax_event_get_data(ds, t->buff, t->handle.size);
        if(result > 0) {
            t->new_data = 1; /* This flag tells run_script() that we have new data */
        }
    }

    pthread_mutex_lock(&q_lock);
    if(q_full) {
        q_overruns++;
        pthread_mutex_unlock(&q_lock);
        dax_log(LOG_ERROR, "Script Queue is Full");
    } else {
        _q_push(script);
        pthread_cond_signal(&q_cond);
        pthread_mutex_unlock(&q_lock);
    }
}

static void
_script_enable_dispatch(dax_state *d, void *udata) {
    script_t *script;

    script = (script_t *)udata;
    script->command = COMMAND_ENABLE;
}

static void
_script_disable_dispatch(dax_state *d, void *udata) {
    script_t *script;

    script = (script_t *)udata;
    script->command = COMMAND_DISABLE;
}


/* Converts the lua_Number that we would get off of the Lua stack into
 * the proper form and assigns it to the write member of the union 'dest'
 * a pointer to this union can then be passed to dax_event_add() as the
 * data argument for EQUAL, GREATER, LESS and DEADBAND events. */
static inline void
_convert_lua_number(tag_type datatype, dax_type_union *dest, lua_Number x) {

    switch(datatype) {
        case DAX_BYTE:
            dest->dax_byte = (dax_byte)x;
            return;
        case DAX_SINT:
        case DAX_CHAR:
            dest->dax_sint = (dax_sint)x;
            return;
        case DAX_UINT:
        case DAX_WORD:
            dest->dax_uint = (dax_uint)x;
            return;
        case DAX_INT:
            dest->dax_int = (dax_int)x;
            return;
        case DAX_UDINT:
        case DAX_DWORD:
        case DAX_TIME:
            dest->dax_udint = (dax_udint)x;
            return;
        case DAX_DINT:
            dest->dax_dint = (dax_dint)x;
            return;
        case DAX_ULINT:
        case DAX_LWORD:
            dest->dax_ulint = (dax_ulint)x;
            return;
        case DAX_LINT:
            dest->dax_lint = (dax_lint)x;
            return;
        case DAX_REAL:
            dest->dax_real = (dax_real)x;
            return;
        case DAX_LREAL:
            dest->dax_lreal = (dax_lreal)x;
            return;
        default:
            dest->dax_ulint = 0L;
    }
    return;
}


/* This function attempts to setup the trigger events with the opendax
   tagserver for each script that has a trigger assigned.  It will return
   the number of scripts where the event creation failed.  This is so that
   the caller will know whether or not to call the function again */
void
_set_trigger_events(void *x) {
    script_t *s;
    trigger_t *t;
    dax_type_union u;
    int count;
    int fails;
    int result;
    long delay = 50000;

    count = get_script_count();

    do {
        fails = 0; /* It'll get set to true later */
        for(int n=0;n<count;n++) {
            s = get_script(n);
            /* If the pointer is not NULL but the size is still zero then we have not
               configured this event yet */
            if(s->script_trigger != NULL && s->script_trigger->handle.size == 0) {
                t = s->script_trigger;
                result = dax_tag_handle(ds, &t->handle, t->tagname, t->count);
                if(result) {
                    fails++;
                } else {
                    _convert_lua_number(t->handle.type, &u, t->value);
                    result = dax_event_add(ds, &t->handle, t->type, (void *)&u,
                                           &t->id, _script_event_dispatch, s, NULL);
                    if(result) {
                        fails++;
                    } else {
                        result = dax_event_options(ds, t->id, EVENT_OPT_SEND_DATA);
                        /* If this is NULL we won't mess with the data in the dispatch function*/
                        t->buff = malloc(t->handle.size);
                    }
                }
            }
            /* Repeat for the enable and disable triggers.  These don't set the data
               return option for the event since that doesn't make any sense for a
               trigger that only sets a flag in the script definition */
            if(s->enable_trigger != NULL && s->enable_trigger->handle.size == 0) {
                t = s->enable_trigger;
                result = dax_tag_handle(ds, &t->handle, t->tagname, t->count);
                if(result) {
                    fails++;
                } else {
                    _convert_lua_number(t->handle.type, &u, t->value);
                    result = dax_event_add(ds, &t->handle, t->type, (void *)&u,
                                           &t->id, _script_enable_dispatch, s, NULL);
                    if(result) {
                        fails++;
                    }
                }
            }
            if(s->disable_trigger != NULL && s->disable_trigger->handle.size == 0) {
                t = s->disable_trigger;
                result = dax_tag_handle(ds, &t->handle, t->tagname, t->count);
                if(result) {
                    fails++;
                } else {
                    _convert_lua_number(t->handle.type, &u, t->value);
                    result = dax_event_add(ds, &t->handle, t->type, (void *)&u,
                                           &t->id, _script_disable_dispatch, s, NULL);
                    if(result) {
                        fails++;
                    }
                }
            }
        }
        usleep(delay);
        delay = delay * 2; /* we get slower each time */
        if(delay > 5e6) delay = 5e6; /* max 5 seconds */
    } while(fails);
    dax_log(LOG_MINOR, "All script trigger events created successfully");
}

static int
_create_thread_types_and_tags(void)
{
    int result, tcount;
    dax_cdt *thread_cdt;
    tag_type thread_type;
    char tagname[DAX_TAGNAME_SIZE+1];
    char tagname2[DAX_TAGNAME_SIZE+20];
    static interval_thread_t *this;

    // TODO: Handle these errors
    thread_cdt = dax_cdt_new("lua_thread", &result);
    result = dax_cdt_member(ds, thread_cdt, "name", DAX_CHAR, DAX_TAGNAME_SIZE);
    result = dax_cdt_member(ds, thread_cdt, "interval", DAX_UDINT, 1);
    result = dax_cdt_member(ds, thread_cdt, "scriptcount", DAX_UDINT, 1);
    result = dax_cdt_member(ds, thread_cdt, "overruns", DAX_UDINT, 1);
    result = dax_cdt_create(ds, thread_cdt, &thread_type);

    /* Count the number of threads that we have */
    this = i_threads;
    tcount = 0;
    while(this != NULL ) {
        tcount++;
        this = this->next;
    }
    snprintf(tagname, DAX_TAGNAME_SIZE+1, "%s_threads", dax_get_attr(ds, "name"));
    result = dax_tag_add(ds, NULL, tagname, thread_type, tcount, TAG_ATTR_READONLY | TAG_ATTR_OWNED);

    this = i_threads;
    for(int n=0;n<tcount;n++) {
        snprintf(tagname2, sizeof(tagname2), "%s[%d]", tagname, n);
        result = dax_tag_handle(ds, &this->handle, tagname2, 0);
        this = this->next;
    }
    return 0;
}


/* The main function that fires off all the threads that we need to
   handle all of the script handling */
int
thread_start_all(void) {
    int result;
    interval_thread_t *this;
    pthread_t t_thread;
    pthread_attr_t attr;

    dax_log(LOG_DEBUG, "Starting all threads");

    _create_thread_types_and_tags();

    this = i_threads;
    while(this != NULL) {
        _start_interval_thread(this);
        this = this->next;
    }

    dax_log(LOG_DEBUG, "Allocating script queue, size = %d", queue_size);
    script_queue = malloc(sizeof(script_t *) * queue_size);
    if(script_queue == NULL) {
        dax_log(LOG_ERROR, "Unable to allocate memory for script queue");
        return ERR_ALLOC;
    }

    result = _start_queue_threads();
    if(result) return result;
    /* This runs each triggered script once.  This allows them to use the _firstrun
       global variable to do some initialization if need be.  It should be understood
       that these were not run because the event was triggered.  This might cause some
       confusion. */
    _run_queue_scripts();

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if(pthread_create(&t_thread, &attr, (void *)&_set_trigger_events, NULL)) {
        dax_log(LOG_ERROR, "Unable to start trigger create thread");
        return -1;
    } else {
        dax_log(LOG_MINOR, "Started trigger event create thread");
        return 0;
    }
    return 0;
}


