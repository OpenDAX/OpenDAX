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


/* script queue threads threads
   These threads are basically worker threads that will take
   scrfipts off of the script queue and run them. */
static unsigned int q_threadcount;
static unsigned int queue_size;
static pthread_t *q_threads;

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
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if(pthread_create(&t->thread, &attr, (void *)&_interval_thread, (void *)t)) {
        dax_log(LOG_ERROR, "Unable to start thread for script - %s", s->name);
        return -1;
    } else {
        dax_log(LOG_MAJOR, "Started Thread for script - %s", s->name);
        return 0;
    }


    return 0;
}

void
thread_start_all(void) {
    interval_thread_t *this;

    dax_log(LOG_DEBUG, "Starting all threads");
    this = i_threads;
    while(this != NULL) {
        _start_interval_thread(this);
        this = this->next;
    }
}


