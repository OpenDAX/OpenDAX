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

add_subdirectory(daxc)

if(BUILD_MODBUS)
  add_subdirectory(modbus)
endif()

if(BUILD_DAXLUA)
  add_subdirectory(daxlua)
endif()

if(BUILD_MQTT)
  add_subdirectory(mqtt)
endif()

if(BUILD_HISTLOG)
  add_subdirectory(histlog)
endif()

if(BUILD_EXAMPLES)
   add_subdirectory(examples)
endif()
