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

#include_directories(.)
add_executable(tagserver server.c
                         options.c
                         func.c
                         module.c
                         message.c
                         tagbase.c
                         crc.c
                         buffer.c
                         events.c
                         mapping.c
                         virtualtag.c
                         groups.c
                         atomic.c
                         retain.c)
target_link_libraries(tagserver ${LUA_LIBRARIES})
target_link_libraries(tagserver pthread)
target_link_libraries(tagserver daxlog)
if(SQLite3_FOUND)
    target_link_libraries(tagserver sqlite3)
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(tagserver PRIVATE -Wall)
endif()
install(TARGETS tagserver DESTINATION bin)
