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

get_target_property(XX daxlua_library OUTPUT_NAME)
set(DAX_PACKAGE_SOURCE "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}${XX}${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(DAX_PACKAGE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
set(DAX_PACKAGE_DESTINATION "${DAX_PACKAGE_DIRECTORY}/dax${CMAKE_SHARED_LIBRARY_SUFFIX}")

message("*******")
message(${DAX_PACKAGE_SOURCE})
message(${DAX_PACKAGE_DIRECTORY})
message(${DAX_PACKAGE_DESTINATION})

# Here we create a symbolic link to the library in a place that the
# Lua interpreter will look for it.
# TODO Need to detect Windows and do a file copy instead
install(DIRECTORY DESTINATION ${DAX_PACKAGE_DIRECTORY})
INSTALL(CODE "execute_process( \
    COMMAND ${CMAKE_COMMAND} -E create_symlink \
    ${DAX_PACKAGE_SOURCE}
    ${DAX_PACKAGE_DESTINATION}   \
    )"
)


# TODO Figure out how to make this just a symbolic link.  This is very cheaty
# and it compiles the silly thing twice.
# add_library(daxlua_package libdaxlua.c)
# target_link_libraries(daxlua_package dax)
# set_target_properties(daxlua_package PROPERTIES OUTPUT_NAME dax)
# 
# install(TARGETS daxlua_package DESTINATION lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR})
