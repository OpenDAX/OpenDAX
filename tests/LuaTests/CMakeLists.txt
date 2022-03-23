#  Copyright (c) 2020 Phil Birkelbach
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# These are various test that call Lua scripts to exercise the Lua libraries

include_directories(../../src/lib)
include_directories(../../src/server)

add_definitions(-DTESTING) # Compiles some testing code into the library */

set(LIB_SOURCE_DIR ../../src/lib)
set(SERVER_SOURCE_DIR ../../src/server)
include_directories(${SERVER_SOURCE_DIR})
include_directories(${LIB_SOURCE_DIR})

add_executable(run_test run_test.c)
add_executable(run_test_event run_test_event.c)
target_link_libraries(run_test_event dax pthread)

add_test(luatest_basic "run_test" "test_basic.lua")
add_test(luatest_tag_delete "run_test" "test_tag_delete.lua")
add_test(luatest_event_wait "run_test_event" "test_event.lua")
