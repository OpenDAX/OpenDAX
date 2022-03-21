/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2022 Phil Birkelbach
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

/*
 *  This test checks for a race condition that would exist when one thread is
 *  waiting on an event and another thread is sending messages to the server.
 *  It would happen because the event_wait function would receive the response
 *  from the other thread's message and then fail.  The other thread would lose
 *  the response.  This race condition will require a rewrite of the library
 *  communication at a fundamental level. */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

int error = 0; /* This is how we determine pass or failure */

void
_wait_callback(dax_state *ds, void* udata) {
    printf("Got the event\n");
}

void *
wait_thread(void *arg) {
    tag_handle h;
    dax_state *ds;
    dax_id id;
    int result;

    ds = arg;
    result = dax_tag_handle(ds, &h, "Test2", 1);

    result = dax_event_add(ds, &h, EVENT_CHANGE, NULL, &id, _wait_callback, NULL, NULL);

    /* We should sit in this wait loop while the other thread does a bunch of communication
     * with the server.  We'll get an error if the race condition shows up. */
    result = dax_event_wait(ds, 1000, &id);
    if(result) {
        printf("Failing because dax_event_wait returned an error\n");
        error = -1;
    } else {
        printf("Got the correct event\n");
    }
    return NULL;
}

void *
tag_thread(void *arg) {
    tag_handle h;
    dax_state *ds;
    dax_dint temp;
    int result;

    ds = arg;
    result = dax_tag_handle(ds, &h, "Test1", 1);
    /* Give the other thread time to get in the event_wait() */
    usleep(100000);
    /* Hammer away */
    for(int n=0; n<500; n++) {
        temp = n*5;
        result = dax_write_tag(ds, h, &temp);
    }
    return NULL;
    result = dax_tag_handle(ds, &h, "Test2", 1);
    temp = 1234;
    /* This should trigger the event in the other thread */
    result = dax_write_tag(ds, h, &temp);
}

static int
_testfunc(int argc, char **argv) {
    tag_handle h1, h2;
    dax_state *ds;
    int result;

    pthread_t _wait_thread, _tag_thread;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    result = dax_tag_add(ds, &h1, "Test1", DAX_DINT, 1, 0);
    if(result) return -1;
    result = dax_tag_add(ds, &h2, "Test2", DAX_DINT, 1, 0);
    if(result) return -1;

    result = pthread_create(&_wait_thread, NULL, wait_thread, ds);
    result = pthread_create(&_tag_thread, NULL, tag_thread, ds);

    pthread_join(_wait_thread, NULL);
    pthread_join(_tag_thread, NULL);
    if(error != 0) {
        printf("Got an error\n");
        return -1;
    } else {
        printf("OK\n");
    }
    return 0;
}

int
run_test(int argc, char **argv) {
    int status = 0;
    int result;
    pid_t pid;

    pid = fork();

    if(pid == 0) { // Child
        execl("../../src/server/tagserver", "../../src/server/tagserver", "-v", NULL);
        printf("Failed to launch tagserver\n");
        exit(-1);
    } else if(pid < 0) {
        exit(-1);
    } else {
        usleep(100000);
        result=_testfunc(argc, argv);
        kill(pid, SIGINT);
        if( waitpid(pid, &status, 0) != pid ) {
            unlink("retentive.db");
            return status;
        }
    }
    unlink("retentive.db");
    return result;
}

/* main inits and then calls run */
int
main(int argc, char *argv[])
{
    if(run_test(argc, argv)) {
        exit(-1);
    } else {
        exit(0);
    }
}
