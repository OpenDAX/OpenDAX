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

add_executable(daxlua_module daxlua.c luaif.c luaopt.c)
set_target_properties(daxlua_module PROPERTIES OUTPUT_NAME daxlua)
target_link_libraries(daxlua_module dax)
target_link_libraries(daxlua_module daxlua_library)
target_link_libraries(daxlua_module pthread)

install(TARGETS daxlua_module DESTINATION bin)
