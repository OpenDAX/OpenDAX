--  Copyright (c) 2020 Phil Birkelbach
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

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
