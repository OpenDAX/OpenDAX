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

# These tests are basically the mqtt module linked with our fake_mqtt.c file instead
# of the paho library.  The module is also compiled with a file that contains at least
# one function that is a thread launched by the MQTTClient_connect() function that
# essentially runs the test by intercepting the publish/subscribe messages and comparing
# results with the tag data. There is a small runner program that is the actual test.  It
# uses the TEST_NAME definition to determine which test to run at compile time.  Each
# test is two executables.  The first is the modified MQTT module and the second
# is trhe runner.   The runner is necessary because we need to launch the tagserver
# as well as the test module
set(test_list raw_subscribe
   )

foreach(test IN LISTS test_list)
  add_executable(${test}  ${test}.c
                          ${CMAKE_SOURCE_DIR}/src/modules/mqtt/mqtt.c
                          ${CMAKE_SOURCE_DIR}/src/modules/mqtt/mqttopts.c
                          fake_mqtt.c)
  target_link_libraries(${test} dax)
  #target_link_libraries(${test} paho-mqtt3c)
  add_executable(mqtt_${test} mqtt_runner.c ../modtest_common.c)
  target_compile_definitions(mqtt_${test} PUBLIC TEST_NAME="${test}")
  add_test(mqtt_${test} mqtt_${test})
  set_tests_properties(mqtt_${test} PROPERTIES TIMEOUT 5)
endforeach()

