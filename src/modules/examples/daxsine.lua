#!/usr/bin/lua
--  Copyright (c) 2022 Phil Birkelbach
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

-- This file outputs sine waves to the given OpenDAX tags.  It's purpose is
-- to simply show how to do such things.  It can also be used as a way
-- to test other modules such as historical logging or HMI

dax = require("dax")

dax.init("dax_sine")

x = 0

-- This calculates sine function and writes the result to the tag server
-- The sine function calculates everything based on the global 'x'.  It
-- is assumed that x will be incremented between calls to this function.
-- The period is how may 'x' ticks will represent a single cycle of the sine
-- wave.  Scale and offset are multipled and then added to the result of
-- of the sine function.  Interval is how often we want to actually write
-- the data to the tag server.  We calculate the interval in this function
-- because it might make sense at some point to write the values to a file
-- or database at some frequency higher than what we write to the tag server.
-- This would help with testing some systems such as historical logging by
-- allowing us to compare the actual values calculated vs the values that
-- are stored.
--
-- handle   = opendax tag handle
-- period   = period of the sine function of second
-- scale    = the scale factor that sine is multiplied without
-- offset   = added to the function
-- interval = interval between writes to the tag server
-- period and interval are based on each time through the loop
function calc(handle, period, scale, offset, interval)
    if x%interval == 0 then
        y = math.sin( (x%period - period/2) / (period/2) * 2*math.pi) * scale + offset
        dax.tag_write(handle, y)
    end
end

-- A table to hold the tags.  The fields are...
--      tagname, period, scale, offset, interval
tags = {
        {"tag1", 500, 5.0, 5, 5},
        {"tag2", 200, 40.0, 0, 4},
        {"tag3", 3000, 60.0, 0, 40},
        {"tag4", 4000, 80.0, 0, 4},
        {"tag5", 5000, 90.0, 0, 4},
        {"tag6", 6000, 100.0, 0, 4},
        {"tag7", 7000, 1.0, 0, 10},
        {"tag8", 10000, 120.0, 0 , 100},
        {"tag9", 10000, 20.0, 0 , 100},
        {"tag10", 5000, 120.0, 0 , 50},
}

-- Create the tags and add the returned handle to the table
for k, tag in pairs(tags) do
    tag[6] = dax.tag_add(tag[1], "REAL")
end

-- go until killed
while true do
    for k, tag in pairs(tags) do
        calc(tag[6], tag[2], tag[3], tag[4], tag[5])
    end
    dax.sleep(10)
    x = x + 1
end
