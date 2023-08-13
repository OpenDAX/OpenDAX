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
 *  Tests for a regression where arrays of CDTs were not retained properly
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

/* We have to run two tests because we need the tag server to exit
   and then start again to test tag retetion.  The first test sets
   everything up and the second test determines if all is retained
   properly */
int
test_one(int argc, char *argv[])
{
    dax_state *ds;
    int result = 0;
    tag_handle h;
    dax_dint t_dint;
    dax_int t_int[12];
    dax_cdt *t;
    tag_type type;

    ds = dax_init("test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    result = 0;

    t = dax_cdt_new("dopey", &result);
    result += dax_cdt_member(ds, t, "this", DAX_DINT, 1);
    result += dax_cdt_member(ds, t, "that", DAX_INT, 12);
    result += dax_cdt_create(ds, t, &type);

    result += dax_tag_add(ds, &h, "dummy", type, 5, TAG_ATTR_RETAIN);
    if(result) return result;

    result = dax_tag_handle(ds, &h, "dummy[0].this", 0);
    t_dint = 1234;
    result += dax_tag_write(ds, h, &t_dint);

    for(int n=0;n<12;n++) t_int[n] = n;
    result += dax_tag_handle(ds, &h, "dummy[4].that", 0);
    result += dax_tag_write(ds, h, &t_int);

    dax_disconnect(ds);

    return result;
}

int
test_two(int argc, char *argv[])
{
    dax_state *ds;
    int result = 0;
    tag_handle h;
    dax_dint t_dint = 0;
    dax_int t_int[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

    ds = dax_init("test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    result = 0;
    result = dax_tag_handle(ds, &h, "dummy[0].this", 0);
    result += dax_tag_read(ds, h, &t_dint);
    if(t_dint != 1234) return -1;

    result += dax_tag_handle(ds, &h, "dummy[4].that", 0);
    result += dax_tag_read(ds, h, &t_int);
    for(int n=0;n<12;n++) if(t_int[n] != n) return -1;

    dax_disconnect(ds);

    return result;
}


int
main(int argc, char *argv[])
{
    if(run_test(test_one, argc, argv, NO_UNLINK_RETAIN)) {
        exit(-1);
    } else {
        if(run_test(test_two, argc, argv, 0)) {
            exit(-1);
        } else {
            exit(0);
        }
    }
}
