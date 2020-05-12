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

# These internal tests compile in different parts of the system and
# tests the functions at a low level.

include_directories(../../src/lib)

add_definitions(-DTESTING) # Compiles some testing code into the library */

set(LIB_SOURCE_DIR ../../src/lib)
add_executable(cachetest cachetest.c ${LIB_SOURCE_DIR}/libdata.c
                                     ${LIB_SOURCE_DIR}/libfunc.c
                                     ${LIB_SOURCE_DIR}/libcdt.c
                                     ${LIB_SOURCE_DIR}/libconv.c
                                     ${LIB_SOURCE_DIR}/libevent.c
                                     ${LIB_SOURCE_DIR}/libinit.c
                                     ${LIB_SOURCE_DIR}/libmsg.c
                                     ${LIB_SOURCE_DIR}/libopt.c
                                     )
# target_link_libraries(cachetest 
target_link_libraries(cachetest ${LUA_LIBRARIES})


