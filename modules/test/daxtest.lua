--This is the main daxtest lua script.  The tests are each hard coded into
--the daxtest module, but the tests are selected and run from this Lua script

PASS = 0
FAIL = 1

--Test that a certain number of tags can be added. Creates the given number
--of tags with random base datatypes and random sizes.
--add_random_tags(3000, "Rand")


--This checks that the tagnames that should fail do so and that tags
--that should be allowed are allowed
fail = {"1Tag", "-Tag", "Tag-name", "Tag&name", "TagNameIsWayTooLong12345678912345"}
pass = {"_Tag", "Tag1", "tAg_name", "t1Ag_name", "TagNameIsBarelyLongEnoughToFit12"}

check_tagnames(fail, pass)

--Test for proper handling of recursive cdt members
cdt_recursion()

--This test checks how members are added to datatypes
--We start by setting up the environment 
test1 = cdt_create("Test1")
test2 = cdt_create("Test2")
test3 = cdt_create("Test3")

cdt_add(test1, "Int5", "INT", 5)     --10
cdt_add(test1, "Bool10", "BOOL", 10) -- 2
cdt_add(test1, "Dint1", "DINT", 1)   -- 4
cdt_add(test1, "Dint3", "DINT", 3)   --12
cdt_finalize(test1)

cdt_add(test2, "Int3", "INT", 3)
cdt_add(test2, "Test1", "Test1", 5)
cdt_finalize(test2)

cdt_add(test3, "Int7", "INT", 7)
cdt_add(test3, "Bool30", "BOOL", 30)
cdt_add(test3, "Bool32", "BOOL", 32)
cdt_add(test3, "Test1", "Test1", 1)
cdt_add(test3, "Test2", "Test2", 2)
cdt_finalize(test3)

tag_add("HandleInt", "INT", 2)
tag_add("HandleTest1", "Test1", 1)
tag_add("HandleTest2", test2, 5)
tag_add("HandleTest3", test3, 1)
tag_add("HandleTest4", test3, 10)

--Test that the handles returned are correct
--Each tag entry contains...
--    the tagname string to parse
--    item count (N)
--    byte offset(B)
--    bit offset (b)
--    byte size  (s)
--    data type  
--    PASS/FAIL
--If FAIL then all arguments are ignored and the string is expected to fail.
--Otherwise the returned handle values must match those given
--  or the test will fail.
--        NAME                N  B   b  s   TYPE     TEST  --
tags = {{"HandleInt[1]",      1, 2,  0, 2,  "INT",   PASS},
        {"HandleInt",         0, 0,  0, 4,  "INT",   PASS},
        {"HandleInt[2]",      5, 0,  0, 0,  "INT",   FAIL},
        {"HandleTest1",       0, 0,  0, 28, "Test1", PASS},
        {"HandleTest1.Dint1", 1, 12, 0, 4,  "DINT",  PASS}}

tag_handle_test(tags)
