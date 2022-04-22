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

# Deleted event test
add_executable(library_event_deleted libtest_event_deleted.c libtest_common.c)
target_link_libraries(library_event_deleted dax)
add_test(library_event_deleted library_event_deleted)
set_tests_properties(library_event_deleted PROPERTIES TIMEOUT 10)

# Event queue tests
add_executable(library_event_queue_simple libtest_event_queue_simple.c libtest_common.c)
target_link_libraries(library_event_queue_simple dax)
add_test(library_event_queue_simple library_event_queue_simple)
set_tests_properties(library_event_queue_simple PROPERTIES TIMEOUT 10)

#add_executable(library_event_queue_overflow1 libtest_event_queue_overflow1.c libtest_common.c)
#target_link_libraries(library_event_queue_overflow1 dax)
#add_test(library_event_queue_overflow1 library_event_queue_overflow1)
#set_tests_properties(library_event_queue_overflow1 PROPERTIES TIMEOUT 10)

#add_executable(library_event_queue_overflow2 libtest_event_queue_overflow2.c libtest_common.c)
#target_link_libraries(library_event_queue_overflow2 dax)
#add_test(library_event_queue_overflow2 library_event_queue_overflow2)
#set_tests_properties(library_event_queue_overflow2 PROPERTIES TIMEOUT 10)

add_executable(library_dax_write_tag_001 libtest_dax_write_tag_001.c libtest_common.c)
target_link_libraries(library_dax_write_tag_001 dax)
add_test(library_dax_write_tag_001 library_dax_write_tag_001)
set_tests_properties(library_dax_write_tag_001 PROPERTIES TIMEOUT 10)

add_executable(library_group_add libtest_group_add.c libtest_common.c)
target_link_libraries(library_group_add dax)
add_test(library_group_add library_group_add)
set_tests_properties(library_group_add PROPERTIES TIMEOUT 10)

add_executable(library_group_read libtest_group_read.c libtest_common.c)
target_link_libraries(library_group_read dax)
add_test(library_group_read library_group_read)
set_tests_properties(library_group_read PROPERTIES TIMEOUT 10)

add_executable(library_group_write libtest_group_write.c libtest_common.c)
target_link_libraries(library_group_write dax)
add_test(library_group_write library_group_write)
set_tests_properties(library_group_write PROPERTIES TIMEOUT 10)

add_executable(library_queue_test libtest_queue_test.c libtest_common.c)
target_link_libraries(library_queue_test dax)
add_test(library_queue_test library_queue_test)
set_tests_properties(library_queue_test PROPERTIES TIMEOUT 10)

add_executable(library_atomic_inc libtest_atomic_inc.c libtest_common.c)
target_link_libraries(library_atomic_inc dax)
add_test(library_atomic_inc library_atomic_inc)
set_tests_properties(library_atomic_inc PROPERTIES TIMEOUT 10)

add_executable(library_atomic_dec libtest_atomic_dec.c libtest_common.c)
target_link_libraries(library_atomic_dec dax)
add_test(library_atomic_dec library_atomic_dec)
set_tests_properties(library_atomic_dec PROPERTIES TIMEOUT 10)

add_executable(library_override_basic libtest_override_basic.c libtest_common.c)
target_link_libraries(library_override_basic dax)
add_test(library_override_basic library_override_basic)
set_tests_properties(library_override_basic PROPERTIES TIMEOUT 10)

add_executable(library_override_get libtest_override_get.c libtest_common.c)
target_link_libraries(library_override_get dax)
add_test(library_override_get library_override_get)
set_tests_properties(library_override_get PROPERTIES TIMEOUT 10)

add_executable(library_retention_basic libtest_retention_basic.c libtest_common.c)
target_link_libraries(library_retention_basic dax)
add_test(library_retention_basic library_retention_basic)
set_tests_properties(library_retention_basic PROPERTIES TIMEOUT 10)

add_executable(library_blank_tagname libtest_blank_tagname.c libtest_common.c)
target_link_libraries(library_blank_tagname dax)
add_test(library_blank_tagname library_blank_tagname)
set_tests_properties(library_blank_tagname PROPERTIES TIMEOUT 10)

add_executable(library_strings libtest_strings.c)
target_link_libraries(library_strings dax)
add_test(library_strings library_strings)
set_tests_properties(library_strings PROPERTIES TIMEOUT 10)

add_executable(library_handles libtest_handles.c libtest_common.c)
target_link_libraries(library_handles dax)
add_test(library_handles library_handles)
set_tests_properties(library_handles PROPERTIES TIMEOUT 10)

