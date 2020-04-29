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

add_executable(daxtest daxtest.c tags.c testlua.c)
target_link_libraries(daxtest dax)
target_link_libraries(daxtest daxlua_library)

add_executable(badmodule badmodule.c)
target_link_libraries(badmodule dax)

configure_file(daxtest.lua daxtest.lua COPYONLY)
file(GLOB files "luatests/*.lua")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(luatests/${FILENAME} luatests/${FILENAME} COPYONLY)
endforeach()
