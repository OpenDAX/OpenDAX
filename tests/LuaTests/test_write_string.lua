-- These tests are designed to exercise the Lua Package API

-- This makes sure that we only load our recently built version
package.cpath = "@CMAKE_BINARY_DIR@/src/lib/lua/?.so"

dax = require("dax")

dax.init("dax_test")

-- Create a tag and write to it
dax.tag_add("ccc", "char", 12)

dax.tag_write("ccc", "This is a string that is way too big to fit")

x = dax.tag_read("ccc")

if x ~= "This is a st" then
    error("Does not match")
end

print("OK")

dax.free()
