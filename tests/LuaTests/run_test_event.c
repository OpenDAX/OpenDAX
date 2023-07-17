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

/* This is a generic test that runs the given Lua script and returns the exit code
   It's just like the run_test program except that this one has a thread that fires
   an event for the Lua script to intercept. */

#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include "../modtest_common.h"

int error = 0;

static
void *_event_thread(void *arg) {
    tag_handle h;
    int n;
    dax_int x;
    dax_state *ds;

    ds = arg;

    n = 0;
    /* Here we wait/loop until the dummy tag is created */
    while(dax_tag_handle(ds, &h, "dummy", 1)) {
        printf(".");
        usleep(100000);
        if(n++ > 20) {
            printf("never found dummy tag\n");
            error = -1;
            return NULL;
        }
    }
    /* Write a value to signal the event */
    x = 4321;
    dax_write_tag(ds, h, &x);
    return NULL;
}

int
main(int argc, char *argv[]) {
    int lua_status, server_status;
    pid_t server_pid;
    pthread_t thread;
    int result;
    char commandline[256];
    dax_state *ds;

    assert(argc >= 2);

    server_pid = run_server();
    if(server_pid < 0) exit(-1);
    usleep(100000);

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);

    result = pthread_create(&thread, NULL, _event_thread, ds);
    if(result) {
        printf("unable to create thread\n");
        exit(-1);
    }

    snprintf(commandline, 256, "%s %s", "lua ", argv[1]);
    lua_status = system(commandline);

    pthread_join(thread, NULL);
    dax_disconnect(ds);

    kill(server_pid, SIGINT);
    if(waitpid(server_pid, &server_status, 0) != server_pid)  {
        printf("Error killing server\n");
    }
    unlink("retentive.db");
    if(lua_status) exit(-1);
    exit(error);
}


