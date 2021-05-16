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
 *  This test creates a single write event and uses dax_event_poll() to
 *  to test whether or not it is called properly
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"


int
do_test(int argc, char *argv[])
{
	dax_state *ds;
    int result = 0;
    u_int32_t idx;
    tag_handle h[100];

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    for(int n=0;n<10;n++) {
        h[n].index = n;
        h[n].size = 4;
        h[n].byte = 0;
        h[n].bit = 0;
        h[n].count = 1;
        h[n].type = DAX_DINT;
    }
    if(result) {
        return -1;
    } else {
        for(int n=0;n<10;n++) {
            result = dax_group_add(ds, &idx, h, 10, 0);
            if(result) return result;
            printf("Test - Added group at index %d\n", idx);
            if(idx != n) return -1;
        }
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
