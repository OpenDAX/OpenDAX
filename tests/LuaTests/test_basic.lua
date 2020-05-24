-- These tests are designed to exercise the Lua Package API

-- This makes sure that we only load our recently build version
package.cpath = "@CMAKE_BINARY_DIR@/src/lib/lua/?.so"


dax = require("dax")

dax.init("dax_test")

-- Create a tag and write to it
dax.tag_add("dummy", "INT")

dax.tag_write("dummy", 12)

-- Make sure that read works with a count argument
x = dax.tag_read("dummy", 1)
if x ~= 12 then
    error("read failure")
end

-- Make sure that read works without a count argument
x = dax.tag_read("dummy")
if x ~= 12 then
    error("read failure")
end

dax.free()
-- io.stderr:write(string.format("%d is the answer\n", x))

