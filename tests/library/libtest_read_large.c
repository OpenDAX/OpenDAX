/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2023 Phil Birkelbach
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
 *  This test reads a tag that is larger than a single message
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
    tag_handle h, h1;
    dax_dint temp;
    dax_dint temp_array[4096];

    ds = dax_init("test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    result = 0;
    result += dax_tag_add(ds, &h, "TEST1", DAX_DINT, 4096, 0);
    if(result) return -1;
    for(int n=0;n<4096;n++) {
        temp_array[n] = n;
    }
    h1.index = h.index;
    h1.type = h.type;
    h1.bit = h.bit;
    h1.count = 512;
    h1.size = 512 * sizeof(dax_dint);
    for(int n=0;n<4096;n+=512) {
        h1.byte = n * sizeof(dax_dint);
        dax_tag_write(ds, h1, &temp_array[n]);
    }

    result = dax_tag_read(ds, h, temp_array);
    if(result != ERR_OK) {
        DF("Read Failure");
        return result;
    }
    for(int n=0;n<4096;n++) {
        if(temp_array[n] != n) {
            DF("Test Failed %d != %d", temp_array[n], n);
            return -1;
        }
    }
    DF("Test Passed");
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

