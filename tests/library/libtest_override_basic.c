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
 *  This does a basic test of the tag override feature
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
    tag_handle h;
    dax_dint temp, n;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    result = 0;
    result += dax_tag_add(ds, &h, "TEST1", DAX_DINT, 1, 0);
    temp = 12;
    result = dax_write_tag(ds, h, &temp);
    if(result) return result;
    temp = -15;
    result = dax_tag_add_override(ds, h, &temp);
    if(result) return result;
    result = dax_read_tag(ds, h, &temp);
    if(result) return result;
    if(temp != 12) return -1;

    result = dax_tag_set_override(ds, h);
    if(result) return result;
    result = dax_read_tag(ds, h, &temp);
    if(result) return result;
    if(temp != -15) return -1;

    result = dax_tag_clr_override(ds, h);
    if(result) return result;
    result = dax_read_tag(ds, h, &temp);
    if(result) return result;
    if(temp != 12) return -1;

    result = dax_tag_set_override(ds, h);
    if(result) return result;
    result = dax_tag_del_override(ds, h);
    if(result) return result;
    result = dax_read_tag(ds, h, &temp);
    if(result) return result;
    if(temp != 12) return -1;

    return result;
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
