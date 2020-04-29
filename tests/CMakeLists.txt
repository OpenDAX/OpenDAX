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

add_subdirectory(modules)

# Compiled C tests
add_executable(testdebug testdebug.c)
target_link_libraries(testdebug dax)
add_test(testdebug testdebug)


# Python Tests

# testconfig.py is imported into all of the Python based tests and it includes
# mostly definitions for where the build system is going to put the files.
configure_file(testconfig.py.in testconfig.py)

# This copies all of the Python test files to the build directory
file(GLOB files "PythonTests/*.py")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(PythonTests/${FILENAME} PythonTests/${FILENAME} COPYONLY)
endforeach()

file(GLOB files "PythonTests/util/*.py")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(PythonTests/util/${FILENAME} PythonTests/util/${FILENAME} COPYONLY)
endforeach()

file(GLOB files "PythonTests/config/*")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(PythonTests/config/${FILENAME} PythonTests/config/${FILENAME} COPYONLY)
endforeach()

# Core system and Library Tests
add_test(python_test_libdax_basic   "python3" "-m" "unittest"
                                    "PythonTests.test_libdax_basic.TestBasic")
add_test(python_test_tag_add        "python3" "-m" "unittest"
                                    "PythonTests.test_tag_add.TestSingle")
add_test(python_test_handles_loop   "python3" "-m" "unittest"
                                    "PythonTests.test_handles.TestHandles_LoopTests")
add_test(python_test_handles_flat   "python3" "-m" "unittest"
                                    "PythonTests.test_handles.TestHandles_FlatTests")
add_test(python_test_libdax_strings "python3" "-m" "unittest"
                                    "PythonTests.test_libdax_strings.TestLibDAXSTrings")
add_test(python_test_mapping        "python3" "-m" "unittest"
                                    "PythonTests.test_mapping.TestMapping")
add_test(python_test_single         "python3" "-m" "unittest"
                                    "PythonTests.test_single.TestSingle")

# Module Tests
add_test(python_test_module_daxc    "python3" "-m" "unittest"
                                    "PythonTests.test_module_daxc.TestDaxc")
if(BUILD_MODBUS)
    add_test(python_test_modbus_library "python3" "-m" "unittest"
    "PythonTests.test_modbuslib.TestRTUMaster")
endif()
add_test(python_test_module_test    "python3" "-m" "unittest"
                                    "PythonTests.test_module_test.TestTestModule")
