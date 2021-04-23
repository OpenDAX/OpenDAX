/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2020 Phil Birkelbach
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
 *  Main source code file for the OpenDAX Bad Module
 */

/* This test program loads up the caching functions from the library and
 * tests that the functions that deal with the cache behave correctly
 */

#include <libcommon.h>
#include "../../src/lib/libdax.h"

/* pushes 16 events to the queue and checks that we can
 * pop them off properly */
int
simple_push_test(void) {
    dax_state *ds;
    dax_message msg;
    dax_message *eq;
    int n, result;

    ds = dax_init("event_queue");
    msg.size = 10;
    msg.msg_type = 20;

    for(n=0; n<16; n++) {
        msg.fd = n+1;
        push_event(ds, &msg);
    }
    eq = ds->emsg_queue;
    for(n=0; n<16; n++) {
        assert(eq[n].fd == n+1);
    }
    dax_free(ds);
    return 0;
}

/* this puts three events on the queue and then pops them off to
 * move the pointer up in the list.  Then it adds eight more to the
 * queue to test that it rolls over properly */
int
simple_fragment_test(void) {
    dax_state *ds;
    dax_message msg;
    dax_message *eq, *test;
    int n, result;

    ds = dax_init("event_queue");
    msg.size = 10;
    msg.msg_type = 20;
    /* put three messages on the queue */
    for(n=0; n<3; n++) {
        msg.fd = n+1;
        push_event(ds, &msg);
    }
    eq = ds->emsg_queue;
    for(n=0; n<3; n++) {
        assert(eq[n].fd == n+1);
    }
    /* pop them off */
    for(n=0; n<3; n++) {
        test = pop_event(ds);
        assert(test == &eq[n]);
    }
    /* Add 8 more */
    for(n=0; n<8; n++) {
        msg.fd = n+11;
        push_event(ds, &msg);
    }
    /* Check that we get them properly */
    for(n=0; n<8; n++) {
        test = pop_event(ds);
        assert(test->fd == n+11);
    }
    dax_free(ds);
    return 0;
}

int
fragment_grow_test(void) {
    dax_state *ds;
    dax_message msg;
    dax_message *eq, *test;
    int n, result;

    ds = dax_init("event_queue");
    msg.size = 10;
    msg.msg_type = 20;
    /* put three messages on the queue */
    for(n=0; n<3; n++) {
        msg.fd = n+1;
        push_event(ds, &msg);
    }
    eq = ds->emsg_queue;
    for(n=0; n<3; n++) {
        assert(eq[n].fd == n+1);
    }
    /* pop them off */
    for(n=0; n<3; n++) {
        test = pop_event(ds);
        assert(test == &eq[n]);
    }
    /* Add 12 more */
    for(n=0; n<12; n++) {
        msg.fd = n+11;
        push_event(ds, &msg);
    }
    /* Check that we get them properly */
    for(n=0; n<12; n++) {
        test = pop_event(ds);
        assert(test->fd == n+11);
    }
    dax_free(ds);
    return 0;
}


int
main(int argc, char *argv[])
{
    int result;

    result = simple_push_test();
    if(result) return result;
    result = simple_fragment_test();
    if(result) return result;
    result = fragment_grow_test();
    if(result) return result;
    return 0;
}
