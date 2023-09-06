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


int
do_test(int argc, char *argv[])
{
    dax_state *ds;
    int result = 0;
    tag_handle h1, h2;
    dax_uint temp, n, mask;
    dax_id id;

    ds = dax_init("test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    result = 0;
    result += dax_tag_add(ds, &h1, "TEST1", DAX_BOOL, 16, 0);
    if(result) return -1;
    result += dax_tag_add(ds, &h2, "TEST2", DAX_UINT, 1, 0);
    if(result) return -1;

    result = dax_map_add(ds, &h1, &h2, &id);
    if(result) return -1;

    temp = 0x0001;
    mask = 0xFFFF;
    DF("Writing 0x%X to first tag", temp);
    result = dax_tag_handle(ds, &h1, "TEST1[0]", 0);
    if(result) return -1;
    result = dax_tag_mask(ds, h1, &temp, &mask);
    if(result) return -1;

    /* Read from the second tag */
    result = dax_tag_read(ds, h2, &temp);
    if(result) return -1;

    if(temp != 0x0001) {
        DF("Returned tag does not match - 0x%X", temp);
        return -1;
    }

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
