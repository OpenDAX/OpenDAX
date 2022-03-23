#  Copyright (c) 2022 Phil Birkelbach
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

# These are miscellaneous tests that didn't really fit anywhere else

include_directories(../../src/lib)
include_directories(../../src/server)

add_definitions(-DTESTING) # Compiles some testing code into the library */

set(LIB_SOURCE_DIR ../../src/lib)
set(SERVER_SOURCE_DIR ../../src/server)

# Event wait race condition test
add_executable(misc_event_wait_race misc_event_wait_race.c)
target_link_libraries(misc_event_wait_race dax pthread)
add_test(misc_event_wait_race misc_event_wait_race)
set_tests_properties(misc_event_wait_race PROPERTIES TIMEOUT 10)

