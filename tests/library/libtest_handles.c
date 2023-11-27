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
        DF("Testing %s", tests_pass[n].tagname);
        type = dax_string_to_type(ds, tests_pass[n].type);
        result = dax_tag_handle(ds, &h, tests_pass[n].tagname, tests_pass[n].count_request);
        if(result) {
            printf(" - dax_tag_handle returned %d for tag = %s\n", result, tests_pass[n].tagname);
            return result;
        }
        if(tests_pass[n].byte != h.byte) {
            printf(" - Byte received %d != %d for tag = %s\n", h.byte, tests_pass[n].byte, tests_pass[n].tagname);
            return 1;
        }
        if(tests_pass[n].bit != h.bit) {
            printf(" - Bit received %d != %d for tag = %s\n", h.bit, tests_pass[n].bit, tests_pass[n].tagname);
            return 1;
        }
        if(tests_pass[n].count != h.count) {
            printf(" - Count received %d != %d for tag = %s\n", h.count, tests_pass[n].count, tests_pass[n].tagname);
            return 1;
        }
        if(tests_pass[n].size != h.size) {
            printf(" - Size received %d != %d for tag = %s\n", h.size, tests_pass[n].size, tests_pass[n].tagname);
            return 1;
        }
        if(type != h.type) {
            printf(" - Type received %d != %d for tag = %s\n", h.type, type, tests_pass[n].tagname);
            return 1;
        }
    }

    count = sizeof(tests_fail) / sizeof(struct handle_test_t);
    printf("Testing for failing handles\n");
    for(n=0;n<count;n++) {
        //printf("Testing %s\n", tests_fail[n].tagname);
        type = dax_string_to_type(ds, tests_fail[n].type);
        result = dax_tag_handle(ds, &h, tests_fail[n].tagname, tests_fail[n].count_request);
        /* These should fail */
        if(result == 0) {
            printf("Testing %s should have failed\n", tests_fail[n].tagname);
            return 1;
        }
    }

    char *flat_tests[] = {
                    "HandleBool8.3",
                    "HandleReal.3",
                    "HandleReal16[2].3",
                    "HandleTest2[0].Test1[0].4",
                    NULL
    };
    /* Flat Tests */
    n=0;
    printf("Running Flat Tests\n");
    while(flat_tests[n] != NULL) {
        printf("Testing %s\n", flat_tests[n]);
        result = dax_tag_handle(ds, &h, flat_tests[n], 1);
        if(result == 0) {
            printf("%s should have failed\n", flat_tests[n]);
            return 1;
        }
        n++;
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
