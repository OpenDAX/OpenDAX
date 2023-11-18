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
 * Header file with common handle testing functions and lists
 */

struct handle_test_t {
    char *tagname;
    int count_request;
    uint32_t byte;
    uint8_t bit;
    uint32_t count;
    uint32_t size;
    char *type;
    char *fqn; /* If not NULL the fqn test will use this instead of tagname */
};

tag_type test1;
tag_type test2;
tag_type test3;
tag_type test4;
tag_type test5;

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
            {"HandleBool1", 0, 0, 0, 1, 1, "BOOL", NULL},
            {"HandleBool7", 0, 0, 0, 7, 1, "BOOL", NULL},
            {"HandleBool7[2]", 4, 0, 2, 4, 1, "BOOL", NULL},
            {"HandleBool7[6]", 1, 0, 6, 1, 1, "BOOL", NULL},
            {"HandleBool7[5]", 2, 0, 5, 2, 1, "BOOL", NULL},
            {"HandleBool8", 0, 0, 0, 8, 1, "BOOL", NULL},
            {"HandleBool9", 0, 0, 0, 9, 2, "BOOL", NULL},
            {"HandleBool9[8]", 0, 1, 0, 1, 1, "BOOL", NULL},
            {"HandleBool33", 0, 0, 0, 33, 5, "BOOL", NULL},
            {"HandleBool33", 8, 0, 0, 8, 1, "BOOL", NULL},
            {"HandleBool33[7]", 1, 0, 7, 1, 1, "BOOL", NULL},
            {"HandleBool33[3]", 8, 0, 3, 8, 2, "BOOL", NULL},
            {"HandleBool33[3]", 9, 0, 3, 9, 2, "BOOL", NULL},
            {"HandleBool33[3]",16, 0, 3, 16, 3, "BOOL", NULL},
            {"HandleInt2[0]", 1, 0, 0, 1, 2, "INT", NULL},
            {"HandleInt2[1]", 1, 2, 0, 1, 2, "INT", NULL},
            {"HandleInt2", 0, 0, 0, 2, 4, "INT", NULL},
            {"HandleInt", 1, 0, 0, 1, 2, "INT", NULL},
            {"HandleInt32", 1, 0, 0, 1, 2, "INT", "HandleInt32[0]"},
            {"HandleInt32[0]", 1, 0, 0, 1, 2, "INT", NULL},
            {"HandleInt32", 0, 0, 0, 32, 64, "INT", NULL},
            {"HandleTest1", 0, 0, 0, 1, 28, "Test1", NULL},
            {"HandleTest1.Dint3", 0, 16, 0, 3, 12, "DINT", NULL},
            {"HandleTest1.Dint3[0]", 2, 16, 0, 2, 8, "DINT", NULL},
            {"HandleTest1.Dint3[1]", 2, 20, 0, 2, 8, "DINT", NULL},
            {"HandleTest1.Dint3[2]", 1, 24, 0, 1, 4, "DINT", NULL},
            {"HandleTest1.Dint1", 1, 12, 0, 1, 4, "DINT", NULL},
            {"HandleTest2[0].Test1[2]", 1, 62, 0, 1, 28, "Test1", NULL},
            {"HandleTest2[0].Test1[1]", 2, 34, 0, 2, 56, "Test1", NULL},
            {"HandleTest2[0].Test1[4]", 1, 118, 0, 1, 28, "Test1", NULL},
            {"HandleTest2[1].Test1", 0, 152, 0, 5, 140, "Test1", NULL},
            {"HandleTest2[4].Int3", 3, 584, 0, 3, 6, "INT", NULL},
            {"HandleTest2[4].Int3", 0, 584, 0, 3, 6, "INT", NULL},

            {"HandleTest2[0].Test1[0].Bool10[4]", 1, 16, 4, 1, 1, "BOOL", NULL},
            {"HandleTest2[0].Test1[0].Bool10", 0, 16, 0, 10, 2, "BOOL", NULL},
            {"HandleTest2[0].Test1[1].Bool10", 0, 44, 0, 10, 2, "BOOL", NULL},
            {"HandleTest2[4].Test1[0].Bool10", 0, 600, 0, 10, 2, "BOOL", NULL},
            {"HandleTest2[1]", 0, 146, 0, 1, 146, "Test2", NULL},

            {"HandleTest4[2].Test1.Bool10", 0, 1580, 0, 10, 2, "BOOL", NULL},
            {"HandleTest4[2].Test1.Bool10", 10, 1580, 0, 10, 2, "BOOL", NULL},

            {"HandleTestIV.Bool", 0, 2, 0, 1, 1, "BOOL", NULL},
            {"HandleTestIV.reBool", 0, 2, 1, 1, 1, "BOOL", NULL},
            {"HandleTestIV.triBool", 0, 2, 2, 1, 1, "BOOL", NULL},
            {"HandleTest5", 0, 0, 0, 1, 2, "Test5", NULL},
            {"HandleTest5.Bool1", 0, 0, 0, 10, 2, "BOOL", NULL},
            {"HandleTest5.Bool1[1]", 0, 0, 1, 1, 1, "BOOL", NULL},
            {"HandleTest5.Bool1[9]", 0, 1, 1, 1, 1, "BOOL", NULL},
            {"HandleTest5.Bool2", 0, 1, 2, 1, 1, "BOOL", NULL},
            {"HandleTest5.Bool3", 0, 1, 3, 3, 1, "BOOL", NULL},
            {"HandleTest5.Bool3[0]", 0, 1, 3, 1, 1, "BOOL", NULL},
            {"HandleTest5.Bool3[2]", 0, 1, 5, 1, 1, "BOOL", NULL},
            {"HandleInt.0", 0, 0, 0, 1, 1, "BOOL", NULL},
            {"HandleInt.5", 0, 0, 5, 1, 1, "BOOL", NULL},
            {"HandleInt.12", 0, 1, 4, 1, 1, "BOOL", NULL},
            {"HandleInt.12", 4, 1, 4, 4, 1, "BOOL", NULL},
            {"HandleInt.15", 0, 1, 7, 1, 1, "BOOL", NULL},
            {"HandleTest1.Int5[0].5", 0, 0, 5, 1, 1, "BOOL", NULL},
            {"HandleTest1.Dint3[0].5", 0, 16, 5, 1, 1, "BOOL", NULL},
            {"HandleTest1.Dint3[0].15", 0, 17, 7, 1, 1, "BOOL", NULL}
    };
    struct handle_test_t tests_fail[] = {
            /* Tag          N  B  b  c  s   type */
            {"HandleBool7[6]", 2, 0, 6, 2, 1, "BOOL", NULL},
            {"HandleBool7[7]", 1, 0, 2, 4, 1, "BOOL", NULL},
            {"HandleBool33[3a]", 1, 0, 0, 0, 0, "BOOL", NULL},
            {"HandleInt[2]", 1, 0, 0, 0, 0, "INT", NULL},
            {"HandleInt[2]", 5, 0, 0, 0, 0, "INT", NULL},
            {"HandleTest2[0].Test1[1]", 5, 32, 0, 1, 28, "Test1", NULL},
            {"HandleTest1.Dint3[2]", 2, 24, 0, 1, 4, "DINT", NULL},
            {"HandleTest2[0].NotAMember", 0, 0, 0, 0, 0, "Yup", NULL},
            {"NoTagName", 0, 0, 0, 0, 0, "Duh", NULL},
            {"HandleInt.16", 0, 1, 4, 1, 1, "BOOL", NULL},
            {"HandleInt.15", 2, 1, 4, 1, 1, "BOOL", NULL},
            {"HandleInt32[0].15", 2, 1, 4, 1, 1, "BOOL", NULL},
            {"HandleInt.6e", 1, 1, 4, 1, 1, "BOOL", NULL},
            {"HandleInt2.7", 2, 1, 4, 1, 1, "BOOL", NULL},
            {"HandleTest1.Dint3.5", 0, 16, 5, 1, 1, "BOOL", NULL},
            {"", 0, 0, 0, 0, 0, "Yup", NULL}
    };


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


static int
_create_types(dax_state *ds) {
    dax_cdt *cdt;
    int result = 0;

    cdt = dax_cdt_new("Test1", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int5", DAX_INT, 5);     // 10
    result += dax_cdt_member(ds, cdt, "Bool10", DAX_BOOL, 10); // 2
    result += dax_cdt_member(ds, cdt, "Dint1", DAX_DINT, 1);   // 4
    result += dax_cdt_member(ds, cdt, "Dint3", DAX_DINT, 3);   // 12
    result += dax_cdt_create(ds, cdt, &test1); /* Size = 28 */
    if(result) return result;

    cdt = dax_cdt_new("Test2", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int3", DAX_INT, 3);
    result += dax_cdt_member(ds, cdt, "Test1", test1, 5);
    result += dax_cdt_create(ds, cdt, &test2); /* Size = 146 */
    if(result) return result;

    cdt = dax_cdt_new("Test3", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int7", DAX_INT, 5);     // 10
    result += dax_cdt_member(ds, cdt, "Bool30", DAX_BOOL, 30); // 4
    result += dax_cdt_member(ds, cdt, "Bool32", DAX_BOOL, 32); // 4
    result += dax_cdt_member(ds, cdt, "Test1", test1, 1);      // 28
    result += dax_cdt_member(ds, cdt, "Test2", test2, 5);      // 146
    result += dax_cdt_create(ds, cdt, &test3); // size = 776
    if(result) return result;

    cdt = dax_cdt_new("Test4", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Int", DAX_INT, 1);      // 2
    result += dax_cdt_member(ds, cdt, "Bool", DAX_BOOL, 1);    // 1
    result += dax_cdt_member(ds, cdt, "reBool", DAX_BOOL, 1);  // 0
    result += dax_cdt_member(ds, cdt, "triBool", DAX_BOOL, 1); // 0
    result += dax_cdt_member(ds, cdt, "Dint", DAX_DINT, 1);    // 4
    result += dax_cdt_create(ds, cdt, &test4); // size = 7
    if(result) return result;

    cdt = dax_cdt_new("Test5", NULL);
    if(cdt == NULL) return 1;
    result += dax_cdt_member(ds, cdt, "Bool1", DAX_BOOL, 10); // 2
    result += dax_cdt_member(ds, cdt, "Bool2", DAX_BOOL, 1);  // 0
    result += dax_cdt_member(ds, cdt, "Bool3", DAX_BOOL, 3);  // 0
    result += dax_cdt_create(ds, cdt, &test5); // size = 2
    if(result) return result;
    return result;
}
