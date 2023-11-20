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
io.stderr:write(string.format("%d is the answer\n", x))
