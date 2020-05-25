-- These tests are designed to exercise the Lua Package API

-- This makes sure that we only load our recently build version
package.cpath = "@CMAKE_BINARY_DIR@/src/lib/lua/?.so"


dax = require("dax")

dax.init("dax_test")

-- Create a tag and write to it
dax.tag_add("dummy", "INT")
dax.tag_write("dummy", 12)

x = dax.tag_read("dummy", 1)
if x ~= 12 then
    error("read failure")
end

dax.tag_del("dummy")
exception = pcall(dax.tag_write, "dummy", 15)

if exception then
    error("Write should have failed")
end

-- TODO: Make sure events are deleted as well as mappings
