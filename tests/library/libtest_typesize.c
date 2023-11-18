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

    result = dax_get_typesize(ds, DAX_BOOL);
    if(result != 1) { DF("bool returned %d", result); return -1; }
    /* 8 Bits */
    result = dax_get_typesize(ds, DAX_BYTE);
    if(result != 1) return -1;
    result = dax_get_typesize(ds, DAX_SINT);
    if(result != 1) return -1;
    result = dax_get_typesize(ds, DAX_CHAR);
    if(result != 1) return -1;
    /* 16 Bits */
    result = dax_get_typesize(ds, DAX_WORD);
    if(result != 2) return -1;
    result = dax_get_typesize(ds, DAX_INT);
    if(result != 2) return -1;
    result = dax_get_typesize(ds, DAX_UINT);
    if(result != 2) return -1;

    /* 32 Bits */
    result = dax_get_typesize(ds, DAX_DWORD);
    if(result != 4) return -1;
    result = dax_get_typesize(ds, DAX_DINT);
    if(result != 4) return -1;
    result = dax_get_typesize(ds, DAX_UDINT);
    if(result != 4) return -1;
    result = dax_get_typesize(ds, DAX_REAL);
    if(result != 4) return -1;
    /* 64 Bits */
    result = dax_get_typesize(ds, DAX_LWORD);
    if(result != 8) return -1;
    result = dax_get_typesize(ds, DAX_LINT);
    if(result != 8) return -1;
    result = dax_get_typesize(ds, DAX_ULINT);
    if(result != 8) return -1;
    result = dax_get_typesize(ds, DAX_TIME);
    if(result != 8) return -1;
    result = dax_get_typesize(ds, DAX_LREAL);
    if(result != 8) return -1;

    result = dax_get_typesize(ds, test1);
    if(result != 28) {
        return -1;
    }
    result = dax_get_typesize(ds, test2);
    if(result != 146) {
        return -1;
    }
    result = dax_get_typesize(ds, test3);
    if(result != 776) {
        return -1;
    }
    result = dax_get_typesize(ds, test4);
    if(result != 7) {
        return -1;
    }
    result = dax_get_typesize(ds, test5);
    if(result != 2) {
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
        DF("Test Fail");
        exit(-1);
    } else {
        DF("Test Pass");
        exit(0);
    }
}
