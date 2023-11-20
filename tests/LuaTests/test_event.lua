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

function fire_event(a)
    io.stderr:write(string.format("Test event called\n"))
    dax.tag_write("dummy", 1234)
end

dax.init("dax_test")

dax.tag_add("dummy", "INT", 1)
dax.tag_write("dummy", 12)

dummy_change = dax.event_add("dummy", 1, "CHANGE", 0, fire_event, 0)

x = dax.event_wait(2000)
io.stderr:write(string.format("%d is the answer\n", x))
if x == 0 then
    error("Timeout")
end

x = dax.tag_read("dummy", 1)
io.stderr:write(string.format("%d is the answer\n", x))

if x ~= 1234 then
    error("Don't match")
end
