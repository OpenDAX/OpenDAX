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

add_executable(mqtt_module mqtt.c mqttopts.c)
set_target_properties(mqtt_module PROPERTIES OUTPUT_NAME daxmqtt)
target_link_libraries(mqtt_module dax)
target_link_libraries(mqtt_module pthread)
target_link_libraries(mqtt_module paho-mqtt3c)


install(TARGETS mqtt_module DESTINATION bin)
