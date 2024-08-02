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

# These tests exercise the system at the library call level.  The tag server
# is started and then various library functions are called to test different
# aspects of the system.

include_directories(../../src/lib)
include_directories(../../src/server)

add_definitions(-DTESTING) # Compiles some testing code into the library */

set(LIB_SOURCE_DIR ../../src/lib)
set(SERVER_SOURCE_DIR ../../src/server)


set(test_list read_large
              write_large
              mask_large
              event_wait
              event_write
              event_change
              event_set_simple
              event_set_multiple
              event_multiple
              event_data
              event_deleted
              event_queue_simple
              # event_queue_overflow1
              # event_queue_overflow2
              dax_write_tag_001
              group_add
              group_read
              group_write
              queue_test
              atomic_inc
              atomic_dec
              atomic_not
              atomic_or
              atomic_and
              atomic_nor
              atomic_nand
              atomic_xor
              atomic_xnor
              override_basic
              override_get
              retention_basic
              retention_cdt
              blank_tagname
              del_system_tag
              strings
              typesize
              type_mem_count
              handles
              fqn
              mapping_basic
              mapping_bool
              mapping_get
              mapping_2way
   )

foreach(test IN LISTS test_list)
    add_executable(library_${test}  libtest_${test}.c libtest_common.c)
    target_link_libraries(library_${test} dax)
    add_test(library_${test} library_${test})
  set_tests_properties(library_${test} PROPERTIES TIMEOUT 5)
endforeach()
