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
 *  This test sends tag handle requests to the server in many different ways and
 *  verifies that the handles that are returned are correct.
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"


#include "handles.h"

int
do_test(int argc, char *argv[])
{
    dax_state *ds;
    int result = 0, count, n;
    tag_type type;
    tag_handle h;
    char *fqn, *str;


    ds = dax_init("test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }

    result = _create_types(ds);
    if(result) return result;

    type = dax_string_to_type(ds, "Test1");
    count = dax_cdt_member_count(ds, type);
    if(count != 4) return -1;

    type = dax_string_to_type(ds, "Test2");
    count = dax_cdt_member_count(ds, type);
    if(count != 2) return -1;

    type = dax_string_to_type(ds, "Test3");
    count = dax_cdt_member_count(ds, type);
    if(count != 5) return -1;

    type = dax_string_to_type(ds, "Test4");
    count = dax_cdt_member_count(ds, type);
    if(count != 5) return -1;

    type = dax_string_to_type(ds, "Test5");
    count = dax_cdt_member_count(ds, type);
    if(count != 3) return -1;

    type = dax_string_to_type(ds, "Test6");
    count = dax_cdt_member_count(ds, type);
    if(count != 3) return -1;

    dax_disconnect(ds);
    return 0;
}

/* main inits and then calls run */
int
main(int argc, char *argv[])
{
    if(run_test(do_test, argc, argv, 0)) {
        DF("Test Fail");
        exit(-1);
    } else {
        DF("Test Pass");
        exit(0);
    }
}
