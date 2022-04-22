#!/usr/bin/lua
-- This file shows what is required to build a very simple OpenDAX
-- client module with Lua

-- import the dax package
dax = require("dax")

-- calling init does the following...
--    * assignes the given name to our module  This name will be reported
--      to the tag server.
--    * allocate all of the data structures that we need
--    * run the configuration system
--    * connect to the tag server
dax.init("dax_basic")

-- The tag_add() function adds a tag to the tag server and returns
-- a handle to that tag.  The parameters are...
--  tagname, datatype, and count (count is optional and is assumed to be 1)
tag1 = dax.tag_add("tag1", "REAL")
tag2 = dax.tag_add("tag2", "BOOL")
tag3 = dax.tag_add("tag3", "DINT", 10) -- Array example
tag4 = dax.tag_add("tag4", "UDINT")

-- we can also retrieve a handle for an existing tag
tag_count = dax.tag_handle("_tagcount")
print("Tag Count = " .. dax.tag_read(tag_count))

dax.tag_write(tag1, 3.141592)
dax.tag_write(tag2, true)
-- Use tables to write to an array
dax.tag_write(tag3, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10})
dax.tag_write(tag4, 0)

pi = dax.tag_read(tag1)
print(pi)

-- We can also skip reading handles and use the tag specifier as a string
--   Using handles is more efficient as the information needed to access
--   the tag is held in the handle.  Using the string might cause the dax
--   library to request tag information from the server, adding to the
--   network traffic.
print("_time = " .. dax.tag_read("_time"))

x = 0 -- loop counter

-- go until killed
while true do
    x = x + 1
    print("count = " .. dax.tag_read(tag4))
    dax.tag_write(tag4, x)
    dax.sleep(1000)
end
