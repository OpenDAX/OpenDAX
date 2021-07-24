/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2021 Phil Birkelbach
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
 *  This test checks that the dax_event_wait() function works properly.  It
 *  is a basic functionality test.  It starts by creating a connection to
 *  the server, creating a tag and assigning a change event to that tag.
 *  Then it forks another process that also connects to the tagbase,
 *  waits a few hunder mSec and then changes that tag externally.
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

int validation = 0;

void
test_callback(dax_state *ds, void *udata) {
    validation++;
}

int
child_function(int argc, char *argv[]) {
    dax_state *ds;
    tag_handle h;
    dax_int x;
    int result;

    ds = dax_init("test2");
    dax_init_config(ds, "test2");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) return result;
    result =  dax_tag_handle(ds, &h, "Dummy", 1);
    if(result) return result;
    /* Pause to make sure we get to wait in the other process */
    usleep(200000);
    x = 12;
    result =  dax_write_tag(ds, h, &x);
    return result;
}

int
do_test(int argc, char *argv[])
{
    pid_t pid;
    dax_state *ds;
    tag_handle tag;
    int result = 0;
    dax_int x;
    dax_id id;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    dax_tag_add(ds, &tag, "Dummy", DAX_INT, 1, 0);
    x = 5;
    dax_write_tag(ds, tag, &x);
    result = dax_event_add(ds, &tag, EVENT_CHANGE, NULL, &id, test_callback, NULL, NULL);
    if(result) return result;

    /* Launch another process to hit our event. */
    pid = fork();

    if(pid == 0) { /* Child */
        child_function(argc, argv);
        exit(0);
    } else if(pid < 0) { /* Error */
        exit(-1);
    } else { /* Parent */
        result = dax_event_wait(ds, 1000, NULL);
        if(result) return result;
        if(validation != 1) return -1;
    }

    return 0;
}

/* main inits and then calls run */
int
main(int argc, char *argv[])
{
    if(run_test(do_test, argc, argv)) {
        exit(-1);
    } else {
        exit(0);
    }
}
