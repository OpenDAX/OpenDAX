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

-- This file outputs an exponential curve to a couple of tags.  This was
-- used to test the historical logging modules CHANGE event algorythm 

dax = require("dax")

dax.init("dax_exp")

x = 0

function calc(tag)
    if tag.direction == 'UP' then
      tag.value = tag.value + tag.value * tag.percent
      if tag.value>100 then tag.direction = 'DOWN' end
    else
      tag.value = tag.value - tag.value * tag.percent
      if tag.value < 0.02 then tag.direction = 'UP' end
    end

    if x%tag.interval == 0 then
        dax.tag_write(tag.handle, tag.value)
    end
end

-- A table to hold the tags.  The fields are...
tags = {
        {name="tag1", value=0.01, percent=0.07, interval=5, direction='UP'},
        {name="tag2", value=0.01, percent=0.07, interval=5, direction='UP'},
}

-- Create the tags and add the returned handle to the table
for k, tag in pairs(tags) do
    tag.handle = dax.tag_add(tag.name, "REAL")
end


-- go until killed
while true do
    for k, tag in pairs(tags) do
        calc(tag)
    end
    dax.sleep(10)
    x = x + 1
end
