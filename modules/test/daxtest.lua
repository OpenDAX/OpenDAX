--This is the main daxtest lua script.  The tests are each hard coded into
--the daxtest module, but the tests are selected and run from this Lua script

PASS = 0
FAIL = 1

run_test("tests/random.lua", "Random Tag Addition Test")
run_test("tests/tagname.lua", "Tagname Addition Test")
run_test("tests/handles.lua", "Tag Handle Retrieval Test")
run_test("tests/status.lua", "Status Retrieve test")
run_test("tests/lazy.lua", "Lazy Programmer Test")

