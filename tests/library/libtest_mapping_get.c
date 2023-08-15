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
 *  This test checks the basic functions of the data mapping
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

static inline int
_handles_equal(tag_handle h1, tag_handle h2)
{
    return(h1.index == h2.index &&
           h1.type == h2.type &&
           h1.count == h2.count &&
           h1.byte == h2.byte &&
           h1.bit == h2.bit &&
           h1.size == h2.size );
}


int
do_test(int argc, char *argv[])
{
    dax_state *ds;
    int result = 0;
    tag_handle h[5];
    tag_handle src, dest;
    dax_dint temp, n;
    dax_id id[4];

    ds = dax_init("test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    if(dax_tag_add(ds, &h[0], "TEST1", DAX_DINT, 1, 0)) return -1;
    if(dax_tag_add(ds, &h[1], "TEST2", DAX_DINT, 1, 0)) return -1;
    if(dax_tag_add(ds, &h[2], "TEST3", DAX_DINT, 1, 0)) return -1;
    if(dax_tag_add(ds, &h[3], "TEST4", DAX_DINT, 1, 0)) return -1;
    if(dax_tag_add(ds, &h[4], "TEST5", DAX_DINT, 1, 0)) return -1;

    if(dax_map_add(ds, &h[0], &h[1], &id[0])) return -1;
    if(dax_map_add(ds, &h[0], &h[2], &id[1])) return -1;
    if(dax_map_add(ds, &h[0], &h[3], &id[2])) return -1;
    if(dax_map_add(ds, &h[0], &h[4], &id[3])) return -1;


    result = dax_map_get(ds, &src, &dest, id[0]);
    if(result) return -1;

    if(!(_handles_equal(src, h[0]) && _handles_equal(dest, h[1]))) {
        DF("TEST FAILED");
        return -1;
    }
    result = dax_map_get(ds, &src, &dest, id[2]);
    if(result) return -1;

    if(!(_handles_equal(src, h[0]) && _handles_equal(dest, h[3]))) {
        DF("TEST FAILED");
        return -1;
    }
    DF("TEST PASSED");
    dax_disconnect(ds);

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
