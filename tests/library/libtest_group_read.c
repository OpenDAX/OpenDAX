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
    tag_group_id *idx;
    tag_handle h[100];
    dax_dint temp;
    dax_dint temp_array[5];

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    result = 0;
    result += dax_tag_add(ds, &h[0], "TEST1", DAX_DINT, 1);
    result += dax_tag_add(ds, &h[1], "TEST2", DAX_DINT, 1);
    result += dax_tag_add(ds, &h[2], "TEST3", DAX_DINT, 1);
    result += dax_tag_add(ds, &h[3], "TEST4", DAX_DINT, 1);
    result += dax_tag_add(ds, &h[4], "TEST5", DAX_DINT, 1);
    if(result) return -1;
    idx = dax_group_add(ds, &result, h, 5, 0);
    if(result) return result;
    printf("Test - Added group at id %p\n", idx);

    result = 0;
    temp = 0x1122;
    result += dax_write_tag(ds, h[0], &temp);
    temp = 0x3344;
    result += dax_write_tag(ds, h[1], &temp);
    temp = 0x5566;
    result += dax_write_tag(ds, h[2], &temp);
    temp = 0x7788;
    result += dax_write_tag(ds, h[3], &temp);
    temp = 0x99AA;
    result += dax_write_tag(ds, h[4], &temp);
    if(result) return -1;
    result = dax_group_read(ds, idx, temp_array, 20);
    for(int n=0;n<5;n++) {
        printf("Array[%d] = 0x%X\n", n, temp_array[n]);
    }
    if(temp_array[0] != 0x1122) return -1;
    if(temp_array[1] != 0x3344) return -1;
    if(temp_array[2] != 0x5566) return -1;
    if(temp_array[3] != 0x7788) return -1;
    if(temp_array[4] != 0x99AA) return -1;
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

