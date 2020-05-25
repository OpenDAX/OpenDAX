--This is the main daxtest lua script.  The tests are each hard coded into
--the daxtest module, but the tests are selected and run from this Lua script

PASS = 0
FAIL = 1

--run_test("luatests/tagname.lua", "Tagname Addition Test")
--run_test("luatests/handles.lua", "Tag Handle Retrieval Test")
run_test("luatests/tagmodify.lua", "Tag Modification Test")
run_test("luatests/typefail.lua", "Type Fail Test")

--run_test("luatests/random.lua", "Random Tag Addition Test")

--run_test("luatests/readwrite.lua", "Read / Write Test")

run_test("luatests/eventadd.lua", "Event Addition/Removal Test")
run_test("luatests/eventwrite.lua", "Event Write Test")
run_test("luatests/eventchange.lua", "Event Change Test")
run_test("luatests/eventset.lua", "Event Set/Reset Test")
run_test("luatests/eventequal.lua", "Event Equal Test")
run_test("luatests/eventgreater.lua", "Event Greater Than Test")
run_test("luatests/eventless.lua", "Event LessThan Test")
run_test("luatests/eventdeadband.lua", "Event Deadband Test")

run_test("luatests/events.lua", "Event Notification Test")

--run_test("tests/lazy.lua", "Lazy Programmer Test")
