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
 *  This test checks that the event queue grows when it's supposed to
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
do_test(int argc, char *argv[])
{
	dax_state *ds;
    tag_handle tag;
    int result = 0;
    dax_int x;
    dax_id id;
    int n;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    } else {
        dax_tag_add(ds, &tag, "Dummy", DAX_INT, 1, 0);
        x = 5;
        dax_write_tag(ds, tag, &x);
        result = dax_event_add(ds, &tag, EVENT_WRITE, NULL, &id, test_callback, NULL, NULL);
        if(result) return result;
        for(n=0;n<12;n++) {
        	x = 5 + n;
        	result = dax_write_tag(ds, tag, &x);
        	if(result) return result;
        }
        for(n=0;n<12;n++) {
            result = dax_event_poll(ds, NULL);
            if(result) return result;
        }
        /* Our Callback will increment validation it runs */
        if(validation != 12) return -1;

    }
    return 0;
}

/* main inits and then calls run */
int
main(int argc, char *argv[])
{
    if(run_test(do_test, argc, argv, 0)) {
        exit(-1);
    } else {
        exit(0);
    }
}
