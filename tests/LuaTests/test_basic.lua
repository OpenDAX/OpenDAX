-- These tests are designed to exercise the Lua Package API

package.cpath = package.cpath .. ";@CMAKE_BINARY_DIR@/src/lib/lua/?.so"

-- This would be "dax" in the installed system but this is the name CMake gives the library so
-- we'll just roll with it for now.
dax = require("dax")

dax.dax_init("dummy")


