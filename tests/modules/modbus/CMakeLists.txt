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

# This is the CList file that takes care of all of the compartmentalized tests
# Most of the tests are written in Python and use the ctypes module to
# interface with the libraries.

# To run an individual test...
# ctest -R <testname> -V

file(GLOB files "conf/*")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(conf/${FILENAME} conf/${FILENAME})
endforeach()

# Check server responses
add_executable(module_modbus_server_responses modtest_server_responses.c ../modtest_common.c)
target_link_libraries(module_modbus_server_responses dax)
add_test(module_modbus_server_responses module_modbus_server_responses)
set_tests_properties(module_modbus_server_responses PROPERTIES TIMEOUT 10)

# Test write single coil message
add_executable(module_modbus_server_fc5 modtest_server_fc5.c ../modtest_common.c)
target_link_libraries(module_modbus_server_fc5 dax)
add_test(module_modbus_server_fc5 module_modbus_server_fc5)
set_tests_properties(module_modbus_server_fc5 PROPERTIES TIMEOUT 10)

# Test write single holding register message
add_executable(module_modbus_server_fc6 modtest_server_fc6.c ../modtest_common.c)
target_link_libraries(module_modbus_server_fc6 dax)
add_test(module_modbus_server_fc6 module_modbus_server_fc6)
set_tests_properties(module_modbus_server_fc6 PROPERTIES TIMEOUT 10)
