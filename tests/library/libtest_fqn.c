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
    result = _create_tags(ds);
    if(result) return result;

    count = sizeof(tests_pass) / sizeof(struct handle_test_t);
    printf("Testing for passing handles\n");
    for(n=0;n<count;n++) {
        DF("testing: %s", tests_pass[n].tagname);
        result = dax_tag_handle(ds, &h, tests_pass[n].tagname, tests_pass[n].count_request);
        if(result) {
            return result;
        }
        fqn = dax_fqn(ds, h);
        if(fqn == NULL) {
            DF("%s - FAIL\n", tests_pass[n].tagname);
            return ERR_GENERIC;
        }
        if(tests_pass[n].fqn != NULL) {
            str = tests_pass[n].fqn;
        } else {
            str = tests_pass[n].tagname;
        }
        if(strcmp(fqn, str) == 0) {
            DF("%s - PASS\n", tests_pass[n].tagname);
        } else {
            DF("%s != %s FAIL\n", fqn, str);
        }
        free(fqn);
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
