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


# To run an individual test...
# ctest -R <testname> -V

file(GLOB files "conf/*")
foreach(file ${files})
  get_filename_component(FILENAME ${file} NAME)
  configure_file(conf/${FILENAME} conf/${FILENAME})
endforeach()

message("SRC DIR ${CMAKE_SOURCE_DIR}")

# This builds a test module that is linked with our "fake" MQTT library instead of Paho
add_executable(mqtt_test_module ${CMAKE_SOURCE_DIR}/src/modules/mqtt/mqtt.c
                                ${CMAKE_SOURCE_DIR}/src/modules/mqtt/mqttopts.c
                                fake_mqtt.c
)
target_link_libraries(mqtt_test_module dax)
#add_test(module_modbus_server_responses module_modbus_server_responses)
#set_tests_properties(mqtt_test_module PROPERTIES TIMEOUT 10)

set(test_list mqtt_raw_subscribe
              )

foreach(test IN LISTS test_list)
  add_executable(${test} ${test}.c ../modtest_common.c)
  target_link_libraries(${test} dax)
  target_link_libraries(${test} paho-mqtt3c)
  add_test(${test} ${test})
  set_tests_properties(${test} PROPERTIES TIMEOUT 5)
endforeach()

#add_executable(mqtt_simple_subscribe mqtt_simple_subscribe.c ../modtest_common.c)
#target_link_libraries(mqtt_simple_subscribe dax)
#target_link_libraries(mqtt_simple_subscribe paho-mqtt3c)
#add_test(mqtt_simple_subscribe mqtt_simple_subscribe)
#set_tests_properties(mqtt_simple_subscribe PROPERTIES TIMEOUT 10)
