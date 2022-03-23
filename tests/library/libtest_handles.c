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
 *  verifies that the handles that are returned are correctr.
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

struct handle_test_t {
    char *tagname;
    int count_request;
    uint32_t byte;
    uint8_t bit;
    uint32_t count;
    uint32_t size;
    char *type;
};

tag_type test1;
tag_type test2;
tag_type test3;
tag_type test4;
tag_type test5;


static int
_create_types(dax_state *ds) {
    dax_cdt *cdt;
    int result = 0;

    cdt = dax_cdt_new("Test1", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int5", DAX_INT, 5);
    result += dax_cdt_member(ds, cdt, "Bool10", DAX_BOOL, 10);
    result += dax_cdt_member(ds, cdt, "Dint1", DAX_DINT, 1);
    result += dax_cdt_member(ds, cdt, "Dint3", DAX_DINT, 3);
    result += dax_cdt_create(ds, cdt, &test1);
    if(result) return result;

    cdt = dax_cdt_new("Test2", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int3", DAX_INT, 3);
    result += dax_cdt_member(ds, cdt, "Test1", test1, 5);
    result += dax_cdt_create(ds, cdt, &test2);
    if(result) return result;

    cdt = dax_cdt_new("Test3", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int7", DAX_INT, 5);
    result += dax_cdt_member(ds, cdt, "Bool30", DAX_BOOL, 30);
    result += dax_cdt_member(ds, cdt, "Bool32", DAX_BOOL, 32);
    result += dax_cdt_member(ds, cdt, "Test1", test1, 1);
    result += dax_cdt_member(ds, cdt, "Test2", test2, 5);
    result += dax_cdt_create(ds, cdt, &test3);
    if(result) return result;

    cdt = dax_cdt_new("Test4", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int", DAX_INT, 1);
    result += dax_cdt_member(ds, cdt, "Bool", DAX_BOOL, 1);
    result += dax_cdt_member(ds, cdt, "reBool", DAX_BOOL, 1);
    result += dax_cdt_member(ds, cdt, "triBool", DAX_BOOL, 1);
    result += dax_cdt_member(ds, cdt, "Dint", DAX_DINT, 1);
    result += dax_cdt_create(ds, cdt, &test4);
    if(result) return result;

    cdt = dax_cdt_new("Test5", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Bool1", DAX_BOOL, 10);
    result += dax_cdt_member(ds, cdt, "Bool2", DAX_BOOL, 1);
    result += dax_cdt_member(ds, cdt, "Bool3", DAX_BOOL, 3);
    result += dax_cdt_create(ds, cdt, &test5);
    if(result) return result;
    return result;
}

static int
_create_tags(dax_state *ds) {
    int result = 0;
    tag_handle h;

    result += dax_tag_add(ds, &h, "HandleBool1", DAX_BOOL, 1, 0);
    result += dax_tag_add(ds, &h, "HandleBool7", DAX_BOOL, 7, 0);
    result += dax_tag_add(ds, &h, "HandleBool8", DAX_BOOL, 8, 0);
    result += dax_tag_add(ds, &h, "HandleBool9", DAX_BOOL, 9, 0);
    result += dax_tag_add(ds, &h, "HandleBool33", DAX_BOOL, 33, 0);
    result += dax_tag_add(ds, &h, "HandleInt", DAX_INT, 1, 0);
    result += dax_tag_add(ds, &h, "HandleInt2", DAX_INT, 2, 0);
    result += dax_tag_add(ds, &h, "HandleInt32", DAX_INT, 32, 0);
    result += dax_tag_add(ds, &h, "HandleReal", DAX_REAL, 1, 0);
    result += dax_tag_add(ds, &h, "HandleReal16", DAX_REAL, 16, 0);
    result += dax_tag_add(ds, &h, "HandleTest1", test1, 1, 0);
    result += dax_tag_add(ds, &h, "HandleTest2", test2, 5, 0);
    result += dax_tag_add(ds, &h, "HandleTest3", test3, 1, 0);
    result += dax_tag_add(ds, &h, "HandleTest4", test3, 10, 0);
    result += dax_tag_add(ds, &h, "HandleTestIV", test4, 1, 0);
    result += dax_tag_add(ds, &h, "HandleTest5", test5, 1, 0);
    result += dax_tag_add(ds, &h, "HandleTest5_10", test5, 10, 0);

    return result;
}

int
do_test(int argc, char *argv[])
{
    dax_state *ds;
    int result = 0, count, n;
    tag_type type;
    tag_handle h;

    /*
     Each tag entry contains...
         the tagname string to parse
         item count  (N) - requested by test
      Returned Values
         byte offset (B)
         bit offset  (b)
         item count  (c) - returned by opendax
         byte size   (s)
         data type
    */
    struct handle_test_t tests_pass[] = {
    /*         Tag          N  B  b  c  s   type */
            {"HandleBool1", 0, 0, 0, 1, 1, "BOOL"},
            {"HandleBool7", 0, 0, 0, 7, 1, "BOOL"},
            {"HandleBool7[2]", 4, 0, 2, 4, 1, "BOOL"},
            {"HandleBool7[6]", 1, 0, 6, 1, 1, "BOOL"},
            {"HandleBool7[5]", 2, 0, 5, 2, 1, "BOOL"},
            {"HandleBool8", 0, 0, 0, 8, 1, "BOOL"},
            {"HandleBool9", 0, 0, 0, 9, 2, "BOOL"},
            {"HandleBool9[8]", 0, 1, 0, 1, 1, "BOOL"},
            {"HandleBool33", 0, 0, 0, 33, 5, "BOOL"},
            {"HandleBool33", 8, 0, 0, 8, 1, "BOOL"},
            {"HandleBool33[7]", 1, 0, 7, 1, 1, "BOOL"},
            {"HandleBool33[3]", 8, 0, 3, 8, 2, "BOOL"},
            {"HandleBool33[3]", 9, 0, 3, 9, 2, "BOOL"},
            {"HandleInt2[1]", 1, 2, 0, 1, 2, "INT"},
            {"HandleInt2", 0, 0, 0, 2, 4, "INT"},
            {"HandleInt", 1, 0, 0, 1, 2, "INT"},
            {"HandleInt32", 1, 0, 0, 1, 2, "INT"},
            {"HandleInt32", 0, 0, 0, 32, 64, "INT"},
            {"HandleTest1", 0, 0, 0, 1, 28, "Test1"},
            {"HandleTest1.Dint3", 0, 16, 0, 3, 12, "DINT"},
            {"HandleTest1.Dint3[0]", 2, 16, 0, 2, 8, "DINT"},
            {"HandleTest1.Dint3[1]", 2, 20, 0, 2, 8, "DINT"},
            {"HandleTest1.Dint3[2]", 1, 24, 0, 1, 4, "DINT"},
            {"HandleTest1.Dint1", 1, 12, 0, 1, 4, "DINT"},
            {"HandleTest2[0].Test1[2]", 1, 62, 0, 1, 28, "Test1"},
            {"HandleTest2[0].Test1[1]", 2, 34, 0, 2, 56, "Test1"},
            {"HandleTest2[0].Test1[4]", 1, 118, 0, 1, 28, "Test1"},
            {"HandleTest2[1].Test1", 0, 152, 0, 5, 140, "Test1"},
            {"HandleTest2[0].Test1[0].Bool10[4]", 1, 16, 4, 1, 1, "BOOL"},
            {"HandleTest2[0].Test1[0].Bool10", 0, 16, 0, 10, 2, "BOOL"},
            {"HandleTest2[0].Test1[1].Bool10", 0, 44, 0, 10, 2, "BOOL"},
            {"HandleTest2[4].Test1[0].Bool10", 0, 600, 0, 10, 2, "BOOL"},
            {"HandleTestIV.Bool", 0, 2, 0, 1, 1, "BOOL"},
            {"HandleTestIV.reBool", 0, 2, 1, 1, 1, "BOOL"},
            {"HandleTestIV.triBool", 0, 2, 2, 1, 1, "BOOL"},
            {"HandleTest5", 0, 0, 0, 1, 2, "Test5"},
            {"HandleTest5.Bool1", 0, 0, 0, 10, 2, "BOOL"},
            {"HandleTest5.Bool1[1]", 0, 0, 1, 1, 1, "BOOL"},
            {"HandleTest5.Bool1[9]", 0, 1, 1, 1, 1, "BOOL"},
            {"HandleTest5.Bool2", 0, 1, 2, 1, 1, "BOOL"},
            {"HandleTest5.Bool3", 0, 1, 3, 3, 1, "BOOL"},
            {"HandleTest5.Bool3[0]", 0, 1, 3, 1, 1, "BOOL"},
            {"HandleTest5.Bool3[2]", 0, 1, 5, 1, 1, "BOOL"},
            {"HandleInt.0", 0, 0, 0, 1, 1, "BOOL"},
            {"HandleInt.5", 0, 0, 5, 1, 1, "BOOL"},
            {"HandleInt.12", 0, 1, 4, 1, 1, "BOOL"},
            {"HandleInt.12", 4, 1, 4, 4, 1, "BOOL"},
            {"HandleInt.15", 0, 1, 7, 1, 1, "BOOL"},
            {"HandleTest1.Int5[0].5", 0, 0, 5, 1, 1, "BOOL"},
            {"HandleTest1.Dint3[0].5", 0, 16, 5, 1, 1, "BOOL"},
            {"HandleTest1.Dint3[0].15", 0, 17, 7, 1, 1, "BOOL"}
    };
    struct handle_test_t tests_fail[] = {
            /* Tag          N  B  b  c  s   type */
            {"HandleBool7[6]", 2, 0, 6, 2, 1, "BOOL"},
            {"HandleBool7[7]", 1, 0, 2, 4, 1, "BOOL"},
            {"HandleBool33[3a]", 1, 0, 0, 0, 0, "BOOL"},
            {"HandleInt[2]", 1, 0, 0, 0, 0, "INT"},
            {"HandleInt[2]", 5, 0, 0, 0, 0, "INT"},
            {"HandleTest2[0].Test1[1]", 5, 32, 0, 1, 28, "Test1"},
            {"HandleTest1.Dint3[2]", 2, 24, 0, 1, 4, "DINT"},
            {"HandleTest2[0].NotAMember", 0, 0, 0, 0, 0, "Yup"},
            {"NoTagName", 0, 0, 0, 0, 0, "Duh"},
            {"HandleInt.16", 0, 1, 4, 1, 1, "BOOL"},
            {"HandleInt.15", 2, 1, 4, 1, 1, "BOOL"},
            {"HandleInt32[0].15", 2, 1, 4, 1, 1, "BOOL"},
            {"HandleInt.6e", 1, 1, 4, 1, 1, "BOOL"},
            {"HandleInt2.7", 2, 1, 4, 1, 1, "BOOL"},
            {"HandleTest1.Dint3.5", 0, 16, 5, 1, 1, "BOOL"},
            {"", 0, 0, 0, 0, 0, "Yup"}
    };

    ds = dax_init("test");
    dax_init_config(ds, "test");

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
        //printf("Testing %s\n", tests_pass[n].tagname);
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
