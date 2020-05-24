-- These tests are designed to exercise the Lua Package API

-- This makes sure that we only load our recently build version
package.cpath = "@CMAKE_BINARY_DIR@/src/lib/lua/?.so"

dax = require("dax")

dax.init("helper")

function fire_event(a)
    io.stderr:write(string.format("Helper event called\n"))
    dax.tag_write("dummy", 1234)
end


dax.tag_add("flag", "INT", 1)

flag_change = dax.event_add("flag", 1, "CHANGE", 0, fire_event, 0)

dax.event_wait(0)

