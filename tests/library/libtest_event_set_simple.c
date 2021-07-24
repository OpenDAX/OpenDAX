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

/*  This test checks the set event by creating an array of bits assigning
 *  a set event to them and toggling individual bits to make sure that
 *  we get the event when we are supposed to
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
    printf("Event Hit\n");
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

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    } else {
        dax_tag_add(ds, &tag, "Dummy", DAX_BOOL, 10, 0);
        x = 5;
        dax_write_tag(ds, tag, &x);
        result = dax_event_add(ds, &tag, EVENT_SET, NULL, &id, test_callback, NULL, NULL);
        if(result) return result;
        /* Test the first bit in the array */
        result = dax_tag_handle(ds, &tag, "Dummy[0]", 1);
        if(result) return result;
        x = 1;
        result = dax_write_tag(ds, tag, &x);
        if(result) return result;
        result = dax_event_poll(ds, NULL);
        if(result) return result;
        if(validation != 1) return -1;
        /* Setting it again should not cause the event */
        result = dax_write_tag(ds, tag, &x);
        if(result) return result;
        result = dax_event_poll(ds, NULL);
        if(result != ERR_NOTFOUND) return -1;
        if(validation != 1) return -1;

        /* Test a different bit in the array */
        result = dax_tag_handle(ds, &tag, "Dummy[5]", 1);
        if(result) return result;
        x = 1;
        result = dax_write_tag(ds, tag, &x);
        if(result) return result;
        result = dax_event_poll(ds, NULL);
        if(result) return result;
        if(validation != 2) return -1;

        /* Test that clearing the bit doesn't cause an event */
        result = dax_tag_handle(ds, &tag, "Dummy[5]", 1);
        if(result) return result;
        x = 0;
        result = dax_write_tag(ds, tag, &x);
        if(result) return result;
        result = dax_event_poll(ds, NULL);
        if(result != ERR_NOTFOUND) return -1;
        /* validation should not have changed */
        if(validation != 2) return -1;
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
