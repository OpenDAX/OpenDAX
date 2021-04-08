-- These tests are designed to exercise the Lua Package API

-- This makes sure that we only load our recently build version
package.cpath = "@CMAKE_BINARY_DIR@/src/lib/lua/?.so"

dax = require("dax")

dax.init("helper")

dax.sleep(200)

dax.tag_write("dummy", 35)
