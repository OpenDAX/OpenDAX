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

#include "plctag.h"
#include <signal.h>

extern dax_state *ds;

static unsigned int queue_size;
static pthread_t q_thread;
static int q_thread_id;

static pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t q_cond = PTHREAD_COND_INITIALIZER;
static pthread_attr_t attr;

static struct tagdef **tag_queue;
static int q_first, q_last;
static int q_full, q_empty;
static int q_exit;


/*** Queue Handling Functions ***/
/* Pushes a tagdef pointer onto the queue array
   The q_lock mutex must be held before calling this function
   returns -1 if the queue is full */
int
tag_queue_push(struct tagdef *tag) {
    pthread_mutex_lock(&q_lock);
    if( ! q_full) {
        //DF("pushing %s, q_first = %d, q_last = %d",script->name, q_first, q_last);
        tag_queue[q_last] = tag;
        q_last++;
        if(q_last == q_first) q_full = 1;
        q_empty = 0;
        if(q_last >= queue_size) q_last = 0;
        pthread_cond_signal(&q_cond);
        pthread_mutex_unlock(&q_lock);
    } else { /* If the queue is full */
        return -1;
    }

    return 0;
}


struct tagdef *
tag_queue_pop(void) {
    struct tagdef *x;
    if(q_empty) return NULL;
    x = tag_queue[q_first];

    q_first++;
    if(q_first >= queue_size) q_first = 0;
    if(q_first == q_last) q_empty = 1;
    q_full = 0;
    return x;
}


static void *
__tag_queue_thread(void *num) {
    struct tagdef *tag;
    int result;

    pthread_mutex_lock(&q_lock);
    while(1) {
        if(q_exit) break;
        pthread_cond_wait(&q_cond, &q_lock);

        while(! q_empty) {
            tag = tag_queue_pop(); // Get the job off of the queue <<sorta>>
            pthread_mutex_unlock(&q_lock); // play nice
            if(tag->buff != NULL) {
                result = dax_tag_write(ds, tag->h, tag->buff);
                if(result) {
                    dax_log(DAX_LOG_ERROR, "Failed to write tag to tagserver - %s", dax_errstr(result));
                }
                if(result == ERR_DISCONNECTED) {
                    kill(getpid(), SIGQUIT);
                }
            }
            //DF("Got tag %s", tag->daxtag);
            pthread_mutex_lock(&q_lock); // get it back
        }
    }
    pthread_mutex_unlock(&q_lock); // Give it up when we are done

    return NULL;
}


void
tag_thread_start(void) {
    tag_queue = malloc(sizeof(struct tagdef) * QUEUE_SIZE);
    queue_size = QUEUE_SIZE;

    pthread_attr_init(&attr);
    pthread_create(&q_thread, NULL, __tag_queue_thread, &q_thread_id);
}