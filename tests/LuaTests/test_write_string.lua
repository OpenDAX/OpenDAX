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
dax.tag_add("ccc", "char", 12)

dax.tag_write("ccc", "This is a string that is way too big to fit")

x = dax.tag_read("ccc")

if x ~= "This is a st" then
    error("Does not match")
end

print("OK")

dax.free()
