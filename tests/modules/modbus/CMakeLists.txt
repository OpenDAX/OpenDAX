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

# To run an individual test...
# ctest -R <testname> -V

file(GLOB files "conf/*")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(conf/${FILENAME} conf/${FILENAME})
endforeach()

set(test_list server_responses
              server_exceptions_1
              server_exceptions_2
              server_exceptions_3
              server_exceptions_4
              server_exceptions_5
              server_fc1
              server_fc2
              server_fc3
              server_fc4
              server_fc5
              server_fc6
              server_fc15
              server_fc16
              server_large_coils
              server_large_discretes
              server_large_holding
              server_large_inputs
              rtu_slave_basic
   )

foreach(test IN LISTS test_list)
    add_executable(module_${test}  modtest_${test}.c ../../modtest_common.c modbus_common.c)
    target_link_libraries(module_${test} dax)
    add_test(module_modbus_${test} module_${test})
    set_tests_properties(module_modbus_${test} PROPERTIES TIMEOUT 10)
endforeach()

