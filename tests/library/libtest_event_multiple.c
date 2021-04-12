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
 *  This test causes multiple events to be sent between calls to dax_event_poll()
 *  to verify that they are all recieved properly.
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

dax_state *ds;
int validation = 0;

void
test_callback(void *udata) {
    validation++;
}

int
do_test(int argc, char *argv[])
{
    tag_handle tags[5];
    int result = 0, i;
    char tagname[32];
    dax_int x;
    dax_id id;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    } else {
        /* Setup the tags and events */
        for(i=0; i<5; i++) {
            sprintf(tagname, "Dummy%d", i);
            dax_tag_add(ds, &tags[i], tagname, DAX_INT, 1);
            result = dax_event_add(ds, &tags[i], EVENT_CHANGE, NULL, &id, test_callback, NULL, NULL);
            if(result) return result;
            x = 5 + i;
            dax_write_tag(ds, tags[i], &x);
            if(result) return result;
        }
        /* Cause the events to happen */
        for(i=0; i<5; i++) {
            x = 6+i;
            result = dax_write_tag(ds, tags[i], &x);
            if(result) return result;
        }
        /* We should get exactly five events */
        for(i=0; i<5; i++) {
            result = dax_event_poll(ds, NULL);
            if(result) return result;
            /* Our Callback will set validation to 1 if it runs */
            if(validation != 1+i) return -1;
        }
        /* this should return a not found error if no event was processed */
        result = dax_event_poll(ds, NULL);
        if(result != ERR_NOTFOUND) return result;
        /* Our Callback will set validation to 1 if it runs */
        if(validation != 5) return -1;
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
