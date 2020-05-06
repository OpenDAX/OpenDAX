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

include_directories(.)

add_library(daxlua_library libdaxlua.c)
target_link_libraries(daxlua_library dax)
set_target_properties(daxlua_library PROPERTIES OUTPUT_NAME daxlua)
# Any target that links to this library will have this directory available
# for include files
target_include_directories(daxlua_library PUBLIC .)

# This installs the version of the library that would be used by C programs
install(TARGETS daxlua_library DESTINATION lib)
# This installs the header file
install(FILES libdaxlua.h DESTINATION include)

#set(LUA_PACKAGE_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}/)

# Here we create a symbolic link to the library in a place that the
# Lua interpreter will look for it.
# TODO Need to detect Windows and do a file copy instead
get_target_property(XX daxlua_library OUTPUT_NAME)
set(DAX_PACKAGE_SOURCE "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}${XX}${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(DAX_PACKAGE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
set(DAX_PACKAGE_DESTINATION "${DAX_PACKAGE_DIRECTORY}/dax${CMAKE_SHARED_LIBRARY_SUFFIX}")

install(DIRECTORY DESTINATION ${DAX_PACKAGE_DIRECTORY})
INSTALL(CODE "execute_process( \
    COMMAND ${CMAKE_COMMAND} -E create_symlink \
    ${DAX_PACKAGE_SOURCE}
    ${DAX_PACKAGE_DESTINATION}   \
    )"
)
install(CODE "message(\"-- Created symlink: ${DAX_PACKAGE_DESTINATION} -> ${DAX_PACKAGE_SOURCE}\")")

# This does something similar except that it does it at build time in the build directory so that we can test 
# the dax lua package interface
set(DAXLUA_TEST_SOURCE "${CMAKE_BINARY_DIR}/src/lib/lua/${CMAKE_SHARED_LIBRARY_PREFIX}${XX}${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(DAXLUA_TEST_DESTINATION "${CMAKE_BINARY_DIR}/src/lib/lua/dax${CMAKE_SHARED_LIBRARY_SUFFIX}")


add_custom_command(TARGET daxlua_library
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ${DAXLUA_TEST_SOURCE} ${DAXLUA_TEST_DESTINATION}
                   DEPENDS daxlua_library
                   )
