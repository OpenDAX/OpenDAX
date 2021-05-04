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
add_executable(module_modbus_server_responses modtest_server_responses.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_responses dax)
add_test(module_modbus_server_responses module_modbus_server_responses)
set_tests_properties(module_modbus_server_responses PROPERTIES TIMEOUT 10)

# Check server exceptions
add_executable(module_modbus_server_exceptions_1 modtest_server_exceptions_1.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_exceptions_1 dax)
add_test(module_modbus_server_exceptions_1 module_modbus_server_exceptions_1)
set_tests_properties(module_modbus_server_exceptions_1 PROPERTIES TIMEOUT 10)

# Check server exceptions
add_executable(module_modbus_server_exceptions_2 modtest_server_exceptions_2.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_exceptions_2 dax)
add_test(module_modbus_server_exceptions_2 module_modbus_server_exceptions_2)
set_tests_properties(module_modbus_server_exceptions_2 PROPERTIES TIMEOUT 10)

# Check server exceptions
add_executable(module_modbus_server_exceptions_3 modtest_server_exceptions_3.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_exceptions_3 dax)
add_test(module_modbus_server_exceptions_3 module_modbus_server_exceptions_3)
set_tests_properties(module_modbus_server_exceptions_3 PROPERTIES TIMEOUT 10)

# Check server exceptions
add_executable(module_modbus_server_exceptions_4 modtest_server_exceptions_4.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_exceptions_4 dax)
add_test(module_modbus_server_exceptions_4 module_modbus_server_exceptions_4)
set_tests_properties(module_modbus_server_exceptions_4 PROPERTIES TIMEOUT 10)

# Check server exceptions
add_executable(module_modbus_server_exceptions_5 modtest_server_exceptions_5.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_exceptions_5 dax)
add_test(module_modbus_server_exceptions_5 module_modbus_server_exceptions_5)
set_tests_properties(module_modbus_server_exceptions_5 PROPERTIES TIMEOUT 10)

# Test read coils
add_executable(module_modbus_server_fc1 modtest_server_fc1.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc1 dax)
add_test(module_modbus_server_fc1 module_modbus_server_fc1)
set_tests_properties(module_modbus_server_fc1 PROPERTIES TIMEOUT 10)

# Test read digital inputs
add_executable(module_modbus_server_fc2 modtest_server_fc2.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc2 dax)
add_test(module_modbus_server_fc2 module_modbus_server_fc2)
set_tests_properties(module_modbus_server_fc2 PROPERTIES TIMEOUT 10)

# Test read holding registers
add_executable(module_modbus_server_fc3 modtest_server_fc3.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc3 dax)
add_test(module_modbus_server_fc3 module_modbus_server_fc3)
set_tests_properties(module_modbus_server_fc3 PROPERTIES TIMEOUT 10)

# Test read holding registers
add_executable(module_modbus_server_fc4 modtest_server_fc4.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc4 dax)
add_test(module_modbus_server_fc4 module_modbus_server_fc4)
set_tests_properties(module_modbus_server_fc4 PROPERTIES TIMEOUT 10)

# Test write single coil message
add_executable(module_modbus_server_fc5 modtest_server_fc5.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc5 dax)
add_test(module_modbus_server_fc5 module_modbus_server_fc5)
set_tests_properties(module_modbus_server_fc5 PROPERTIES TIMEOUT 10)

# Test write single holding register message
add_executable(module_modbus_server_fc6 modtest_server_fc6.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc6 dax)
add_test(module_modbus_server_fc6 module_modbus_server_fc6)
set_tests_properties(module_modbus_server_fc6 PROPERTIES TIMEOUT 10)

# Test write multiple coil message
add_executable(module_modbus_server_fc15 modtest_server_fc15.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc15 dax)
add_test(module_modbus_server_fc15 module_modbus_server_fc15)
set_tests_properties(module_modbus_server_fc15 PROPERTIES TIMEOUT 10)

# Test write multiple holding registers message
add_executable(module_modbus_server_fc16 modtest_server_fc16.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_fc16 dax)
add_test(module_modbus_server_fc16 module_modbus_server_fc16)
set_tests_properties(module_modbus_server_fc16 PROPERTIES TIMEOUT 10)

# Test large coil database
add_executable(module_modbus_server_large_coils modtest_server_large_coils.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_large_coils dax)
add_test(module_modbus_server_large_coils module_modbus_server_large_coils)
set_tests_properties(module_modbus_server_large_coils PROPERTIES TIMEOUT 10)

# Test large discrete database
add_executable(module_modbus_server_large_discretes modtest_server_large_discretes.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_large_discretes dax)
add_test(module_modbus_server_large_discretes module_modbus_server_large_discretes)
set_tests_properties(module_modbus_server_large_discretes PROPERTIES TIMEOUT 10)

# Test large discrete database
add_executable(module_modbus_server_large_holding modtest_server_large_holding.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_large_holding dax)
add_test(module_modbus_server_large_holding module_modbus_server_large_holding)
set_tests_properties(module_modbus_server_large_holding PROPERTIES TIMEOUT 10)

# Test large discrete database
add_executable(module_modbus_server_large_inputs modtest_server_large_inputs.c ../modtest_common.c modbus_common.c)
target_link_libraries(module_modbus_server_large_inputs dax)
add_test(module_modbus_server_large_inputs module_modbus_server_large_inputs)
set_tests_properties(module_modbus_server_large_inputs PROPERTIES TIMEOUT 10)
