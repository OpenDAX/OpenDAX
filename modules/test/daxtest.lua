--This is the main daxtest lua script.  The tests are each hard coded into
--the daxtest module, but the tests are selected and run from this Lua script

PASS = 0
FAIL = 1

--Test that a certain number of tags can be added. Creates the given number
--of tags with random base datatypes and random sizes.
--add_random_tags(100000, "Rand")

--This checks that the tagnames that should fail do so and that tags
--that should be allowed are allowed
fail = {"1Tag", "-Tag", "Tag-name", "Tag&name", "TagNameIsWayTooLong12345678912345"}
pass = {"_Tag", "Tag1", "tAg_name", "t1Ag_name", "TagNameIsBarelyLongEnoughToFit12"}

--check_tagnames(fail, pass)


--This test checks how members are added to datatypes
--We start by setting up the environment
members = {{"Int5", "INT", 5},     --10
           {"Bool10", "BOOL", 10}, -- 2
           {"Dint1", "DINT", 1},   -- 4
           {"Dint3", "DINT", 3}}   --12
                                   --28 Total
test1 = cdt_create("Test1", members)

members = {{"Int3", "INT", 3},     --  6
           {"Test1", "Test1", 5}}  --140
                                   --146 Total
test2 = cdt_create("Test2", members)

members = {{"Int7", "INT", 7},      -- 14
           {"Bool30", "BOOL", 30},  --  1
           {"Bool32", "BOOL", 32},  --  1
           {"Test1", "Test1", 1},   -- 28
           {"Test2", "Test2", 2}}   --292
                                    --336 Total

test3 = cdt_create("Test3", members)

tag_add("HandleBool1", "BOOL", 1)
tag_add("HandleBool7", "BOOL", 7)
tag_add("HandleBool8", "BOOL", 8)
tag_add("HandleBool9", "BOOL", 9)
tag_add("HandleBool33", "BOOL", 33)
tag_add("HandleInt", "INT", 2)
tag_add("HandleTest1", "Test1", 1)
tag_add("HandleTest2", test2, 5)
tag_add("HandleTest3", test3, 1)
tag_add("HandleTest4", test3, 10)

--Test that the handles returned are correct
--Each tag entry contains...
--    the tagname string to parse
--    item count  (N) - requested by test
-- Returned Values
--    byte offset (B)
--    bit offset  (b)
--    item count  (c) - returned by opendax
--    byte size   (s)
--    data type  
--    PASS/FAIL
--If FAIL then all arguments are ignored and the string is expected to fail.
--Otherwise the returned handle values must match those given
--  or the test will fail.
--        NAME                          N   B   b  c   s   TYPE      TEST  --
tags = {{"HandleBool1",                 0,  0,  0, 1,  1,  "BOOL",   PASS},
        {"HandleBool7",                 0,  0,  0, 7,  1,  "BOOL",   PASS},
        {"HandleBool7[2]",              4,  0,  2, 4,  1,  "BOOL",   PASS},
        {"HandleBool7[6]",              1,  0,  6, 1,  1,  "BOOL",   PASS},
        {"HandleBool7[5]",              2,  0,  5, 2,  1,  "BOOL",   PASS},
        {"HandleBool7[6]",              2,  0,  6, 2,  1,  "BOOL",   FAIL},
        {"HandleBool7[7]",              1,  0,  2, 4,  1,  "BOOL",   FAIL},
        {"HandleBool8",                 0,  0,  0, 8,  1,  "BOOL",   PASS},
        {"HandleBool9",                 0,  0,  0, 9,  2,  "BOOL",   PASS},
        {"HandleBool9[8]",              0,  1,  0, 1,  1,  "BOOL",   PASS},
        {"HandleBool33",                0,  0,  0, 33, 5,  "BOOL",   PASS},
        {"HandleBool33[7]",             1,  0,  7, 1,  1,  "BOOL",   PASS},
        {"HandleBool33[3]",             8,  0,  3, 8,  1,  "BOOL",   PASS},
        {"HandleBool33[3]",             9,  0,  3, 9,  2,  "BOOL",   PASS},
        {"HandleBool33[3a]",            1,  0,  0, 0,  0,  "BOOL",   FAIL},
        {"HandleInt[1]",                1,  2,  0, 1,  2,  "INT",    PASS},
        {"HandleInt",                   0,  0,  0, 2,  4,  "INT",    PASS},
        {"HandleInt[2]",                1,  0,  0, 0,  0,  "INT",    FAIL},
        {"HandleInt[2]",                5,  0,  0, 0,  0,  "INT",    FAIL},
        {"HandleTest1",                 0,  0,  0, 1,  28, "Test1",  PASS},
        {"HandleTest1.Dint3",           0,  16, 0, 3,  12, "DINT",   PASS},
        {"HandleTest1.Dint3[0]",        2,  16, 0, 2,  8,  "DINT",   PASS},
        {"HandleTest1.Dint3[1]",        2,  20, 0, 2,  8,  "DINT",   PASS},
        {"HandleTest1.Dint3[2]",        1,  24, 0, 1,  4,  "DINT",   PASS},
        {"HandleTest1.Dint3[2]",        2,  24, 0, 1,  4,  "DINT",   FAIL},
        {"HandleTest1.Dint1",           1,  12, 0, 1,  4,  "DINT",   PASS},
        {"HandleTest2[0].Test1[2]",     1,  62, 0, 1,  28, "Test1",  PASS},
        {"HandleTest2[0].Test1[1]",     2,  34, 0, 2,  56, "Test1",  PASS},
        {"HandleTest2[0].Test1[4]",     1,  118,0, 1,  28, "Test1",  PASS},
        {"HandleTest2[0].Test1[1]",     5,  32, 0, 1,  28, "Test1",  FAIL},
        {"HandleTest2[1].Test1",        1,  152,0, 5,  140,"Test1",  PASS},
        {"HandleTest2[0].Test1[0].Bool10[4]",
                                        1,  16, 4, 1,  1,  "BOOL",   PASS},
        {"HandleTest2[0].Test1[0].Bool10", 
                                        0,  16, 0, 10, 2,  "BOOL",   PASS},
        {"HandleTest2[0].Test1[1].Bool10", 
                                        0,  44, 0, 10, 2,  "BOOL",   PASS},
        {"HandleTest2[4].Test1[0].Bool10", 
                                        0,  600,0, 10, 2,  "BOOL",   PASS},
        {"NoTagName",                   0,  0,  0, 0,  0,  "Duh",    FAIL}}
        
tag_handle_test(tags)
