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

# Event wait tests
add_executable(library_event_wait libtest_event_wait.c libtest_common.c)
target_link_libraries(library_event_wait dax)
add_test(library_event_wait library_event_wait)
set_tests_properties(library_event_wait PROPERTIES TIMEOUT 10)

# Write event test
add_executable(library_event_write libtest_event_write.c libtest_common.c)
target_link_libraries(library_event_write dax)
add_test(library_event_write library_event_write)
set_tests_properties(library_event_write PROPERTIES TIMEOUT 10)

# Change event test
add_executable(library_event_change libtest_event_change.c libtest_common.c)
target_link_libraries(library_event_change dax)
add_test(library_event_change library_event_change)
set_tests_properties(library_event_change PROPERTIES TIMEOUT 10)

# Simple Set event test
add_executable(library_event_set_simple libtest_event_set_simple.c libtest_common.c)
target_link_libraries(library_event_set_simple dax)
add_test(library_event_set_simple library_event_set_simple)
set_tests_properties(library_event_set_simple PROPERTIES TIMEOUT 10)

# Simple Set event test
add_executable(library_event_set_multiple libtest_event_set_multiple.c libtest_common.c)
target_link_libraries(library_event_set_multiple dax)
add_test(library_event_set_multiple library_event_set_multiple)
set_tests_properties(library_event_set_multiple PROPERTIES TIMEOUT 10)

# Multiple event tests
add_executable(library_event_multiple libtest_event_multiple.c libtest_common.c)
target_link_libraries(library_event_multiple dax)
add_test(library_event_multiple library_event_multiple)
set_tests_properties(library_event_multiple PROPERTIES TIMEOUT 10)

# Simple event data tests
add_executable(library_event_data libtest_event_data.c libtest_common.c)
target_link_libraries(library_event_data dax)
add_test(library_event_data library_event_data)
set_tests_properties(library_event_data PROPERTIES TIMEOUT 10)

# Event queue tests
add_executable(library_event_queue_simple libtest_event_queue_simple.c libtest_common.c)
target_link_libraries(library_event_queue_simple dax)
add_test(library_event_queue_simple library_event_queue_simple)
set_tests_properties(library_event_queue_simple PROPERTIES TIMEOUT 10)

add_executable(library_event_queue_overflow1 libtest_event_queue_overflow1.c libtest_common.c)
target_link_libraries(library_event_queue_overflow1 dax)
add_test(library_event_queue_overflow1 library_event_queue_overflow1)
set_tests_properties(library_event_queue_overflow1 PROPERTIES TIMEOUT 10)

add_executable(library_event_queue_overflow2 libtest_event_queue_overflow2.c libtest_common.c)
target_link_libraries(library_event_queue_overflow2 dax)
add_test(library_event_queue_overflow2 library_event_queue_overflow2)
set_tests_properties(library_event_queue_overflow2 PROPERTIES TIMEOUT 10)
