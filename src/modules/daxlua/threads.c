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
    //DF("pushing %d, q_first = %d, q_last = %d\n",script, q_first, q_last);
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
    //DF("poping %d, q_first = %d, q_last = %d\n",x, q_first, q_last);
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
        } else {
            usleep(sleep_time); /* Just for now need to calculate the time */
        }

    }

    return NULL;
}

/* This sets up the interpreter with the functions and globals
   and reads the file from the filesystem and stores the chunk
   in the registry. */
void
_initialize_script(script_t *s) {
    /* Create a lua interpreter object */
    setup_interpreter(s->L);
    /* load and compile the file */
    if(luaL_loadfile(s->L, s->filename) ) {
        dax_log(LOG_ERROR, "Error Loading Main Script - %s", lua_tostring(s->L, -1));
        s->failed = 1;
    } else {
        /* Basicaly stores the Lua script */
        s->func_ref = luaL_ref(s->L, LUA_REGISTRYINDEX);
    }
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
            _initialize_script(s);
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
    DF("Thread %d Exiting\n", *(int *)num);

    return NULL;
}


int
_start_queue_threads(void) {
    int n;

    DF("Starting %d queue threads", q_threadcount);
    q_threads = malloc(sizeof(pthread_t) * q_threadcount);
    q_thread_ids = malloc(sizeof(int) * q_threadcount);

    pthread_attr_init(&attr);

    for(n=0;n<q_threadcount;n++) {
        DF("Starting queue thread %d", n);
        q_thread_ids[n] = n;
        pthread_create(&q_threads[n], NULL, __queue_thread, &q_thread_ids[n]);
    }
    return 0;
}

/* This function loops through all of the scripts that have
   .trigger set and initializes them and then pushes them onto
   the queue so that the worker thread will run them. */
void
_run_queue_scripts(void) {
    int count;
    int n = 0;
    script_t *script;

    count = get_script_count();

    while(n<count) {
        script = get_script(n);

        if(script->trigger) {
            _initialize_script(script);
            pthread_mutex_lock(&q_lock);
            if( ! q_full) {
                DF("Pushing script %s onto the queue", script->name);
                _q_push(script);
                pthread_cond_signal(&q_cond);
                pthread_mutex_unlock(&q_lock);
                n++;
            } else { /* If the queue is full */
                DF("Queue full...waiting");
                usleep(50000);
            }
        } else { /* Not an event script */
            n++;
        }
    }
}

static void
_script_event_dispatch(dax_state *d, void *udata) {
    script_t *script;
    int result;

    script = (script_t *)udata;

    if(script->event_buff != NULL) {
        result = dax_event_get_data(ds, script->event_buff, script->event_handle.size);
        if(result > 0) {
            daxlua_dax_to_lua(script->L, script->event_handle, script->event_buff);
            lua_setglobal(script->L, "_eventdata");
        }
    }

    run_script(script);
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
    dax_type_union u;
    dax_id id;
    int count;
    int fails;
    int result;
    long delay = 50000;

    count = get_script_count();

    do {
        fails = 0; /* It'll get set to true later */
        for(int n=0;n<count;n++) {
            s = get_script(n);
            if(s->trigger == 1) { /* 1 means we have a trigger but not setup yet */
                result = dax_tag_handle(ds, &s->event_handle, s->event_tagname, s->event_count);
                if(result) {
                    fails++;
                } else {
                    _convert_lua_number(s->event_handle.type, &u, s->event_value);
                    result = dax_event_add(ds, &s->event_handle, s->event_type, (void *)&u,
                                           &id, _script_event_dispatch, s, NULL);
                    if(result) {
                        fails++;
                    } else {
                        result = dax_event_options(ds, id, EVENT_OPT_SEND_DATA);
                        /* If this is NULL we won't mess with the data in the dispatch function*/
                        s->event_buff = malloc(s->event_handle.size);
                        s->trigger = 2; /* two means we are good to go */
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

/* The main function that fires off all the threads that we need to
   handle all of the script handling */
int
thread_start_all(void) {
    int result;
    interval_thread_t *this;
    pthread_t t_thread;
    pthread_attr_t attr;

    dax_log(LOG_DEBUG, "Starting all threads");
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


