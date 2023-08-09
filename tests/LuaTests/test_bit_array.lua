-- These tests are designed to exercise the Lua Package API

-- This makes sure that we only load our recently built version
package.cpath = "@CMAKE_BINARY_DIR@/src/lib/lua/?.so"



dax = require("dax")

dax.init("dax_test")

-- Create a tag and write to it
dax.tag_add("bits", "bool", 30)

x = dax.tag_read("bits")

for n=1,30 do
  x[n] = false
end

x[4]  = true
x[12] = true
x[21] = true

-- This tests a regression where writing somethign other than a table
-- to a BOOL array would cause a seg fault
f = pcall(dax.tag_write, "bits", 30)
if f == true then
    error("oops")
end
-- This should work
dax.tag_write("bits", x)

-- And just to be sure we'll read it back and compare
y = dax.tag_read("bits")

for n=1,30 do
  if x[n] ~= y[n] then
    error("no match at "..tostring(n))
  end
end

print("OK")

dax.free()
