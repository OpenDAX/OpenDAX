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

# These internal tests link directly to different parts of the system. So that
# they can test functions at a low level.

include_directories(../../src/lib)
include_directories(../../src/server)

add_definitions(-DTESTING) # Compiles some testing code into the library */

set(LIB_SOURCE_DIR ../../src/lib)
set(SERVER_SOURCE_DIR ../../src/server)
include_directories(${SERVER_SOURCE_DIR})
include_directories(${LIB_SOURCE_DIR})

# Library tests
# tests the tag cache in the library
add_executable(cachetest cachetest.c ${LIB_SOURCE_DIR}/libdata.c
                                     ${LIB_SOURCE_DIR}/libfunc.c
                                     ${LIB_SOURCE_DIR}/libcdt.c
                                     ${LIB_SOURCE_DIR}/libconv.c
                                     ${LIB_SOURCE_DIR}/libevent.c
                                     ${LIB_SOURCE_DIR}/libinit.c
                                     ${LIB_SOURCE_DIR}/libmsg.c
                                     ${LIB_SOURCE_DIR}/libopt.c
                                     ${LIB_SOURCE_DIR}/lua/libdaxlua.c
                                     ../testlog.c 
                                     )
# target_link_libraries(cachetest
target_link_libraries(cachetest ${LUA_LIBRARIES})
target_link_libraries(cachetest pthread)

#add_executable(event_queue event_queue.c ${LIB_SOURCE_DIR}/libdata.c
#                                         ${LIB_SOURCE_DIR}/libfunc.c
#                                         ${LIB_SOURCE_DIR}/libcdt.c
#                                         ${LIB_SOURCE_DIR}/libconv.c
#                                         ${LIB_SOURCE_DIR}/libevent.c
#                                         ${LIB_SOURCE_DIR}/libinit.c
#                                         ${LIB_SOURCE_DIR}/libmsg.c
#                                         ${LIB_SOURCE_DIR}/libopt.c
#                                         ${LIB_SOURCE_DIR}/lua/libdaxlua.c
#                                          )
#target_link_libraries(event_queue ${LUA_LIBRARIES})
#target_link_libraries(event_queue pthread)
#add_test(internal_library_event_queue event_queue)

add_test(internal_library_cache cachetest)

set(test_list tagbasetest_001
              tagbasetest_002
              tagbasetest_003
              tagbasetest_004
)

# Server Tests
foreach(test IN LISTS test_list)
  add_executable(${test} ${test}.c fakefunction.c
                                         ${SERVER_SOURCE_DIR}/tagbase.c
                                         ${SERVER_SOURCE_DIR}/func.c
                                         ${SERVER_SOURCE_DIR}/events.c
                                         ${SERVER_SOURCE_DIR}/retain.c
                                         ${SERVER_SOURCE_DIR}/mapping.c
                                         ${SERVER_SOURCE_DIR}/virtualtag.c
                                         ../testlog.c 
  )
  if(SQLite3_FOUND)
    target_link_libraries(${test} sqlite3)
  endif()
  add_test(${test} ${test})

endforeach()

add_executable(groups_test groups_test.c fakefunction.c
                                         ${SERVER_SOURCE_DIR}/groups.c
                                         ${SERVER_SOURCE_DIR}/tagbase.c
                                         ${SERVER_SOURCE_DIR}/func.c
                                         ${SERVER_SOURCE_DIR}/events.c
                                         ${SERVER_SOURCE_DIR}/retain.c
                                         ${SERVER_SOURCE_DIR}/mapping.c
                                         ${SERVER_SOURCE_DIR}/virtualtag.c
                                         ../testlog.c 
  )
if(SQLite3_FOUND)
  target_link_libraries(groups_test sqlite3)
endif()
add_test(internal_server_tag_group groups_test)
