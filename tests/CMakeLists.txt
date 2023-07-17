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

add_compile_definitions(BUILD_DIR="${CMAKE_BINARY_DIR}")

if(INTERNAL_TESTS)
  add_subdirectory(internal)
endif()

add_subdirectory(library)

add_subdirectory(modules)

add_subdirectory(misc)

file(GLOB files "LuaTests/*")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(LuaTests/${FILENAME} LuaTests/${FILENAME})
endforeach()

add_subdirectory(LuaTests)

add_subdirectory(oldtests)

