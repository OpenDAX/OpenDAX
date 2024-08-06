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

include(CheckIncludeFile)

# This allows us to use VERSION in the project command
cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.10.2)
project(OpenDAX VERSION 0.9)

set(BUILD_SHARED_LIBS ON)

enable_testing()

option(BUILD_DOC "build documentation." OFF)
option(BUILD_DAXLUA "Build Lua Module" ON)
option(BUILD_MODBUS "Build Modbus Module" ON)
option(BUILD_MQTT "Build MQTT Module" ON)
option(BUILD_PLCTAG "Build PLCTAG Module" ON)
option(BUILD_HISTLOG "Build Historical Logging Module" ON)
option(BUILD_OPCUA "Build OPC UA Module" ON)
option(BUILD_JOYSTICK "Build Joystick Module" ON)
option(BUILD_EXAMPLES "Build Example Modules" ON)
option(INTERNAL_TESTS "Build and Run Internal System Tests" ON)
option(MODULE_TESTS "Build and Run Module Tests" ON)

set(CMAKE_BUILD_TYPE Debug)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Building Debug Symbols and Verbose Warnings")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(OS_LINUX 1)
endif()

if(EXISTS "/proc")
    set(HAVE_PROCDIR 1)
endif()

check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)

include(FindLua)
# message("Lua Libraries Found: ${LUA_LIBRARIES}")
include_directories(${LUA_INCLUDE_DIR})
if(NOT LUA_LIBRARIES)
  message(FATAL_ERROR "Lua library not found!!")
endif()

message("Lua Version ${LUA_VERSION_STRING} found")
# concatenate Lua major/minor versions together so we can do a comparison
set(LUA_VERSION_TEST "${LUA_VERSION_MAJOR}${LUA_VERSION_MINOR}")
if(${LUA_VERSION_TEST} LESS 53)
  message(FATAL_ERROR "Lua Version 5.3 or later required")
endif()

# SQLite is used in the historical logging module as one of the plugin types
include(FindSQLite3)
if(NOT SQLite3_FOUND)
  message(STATUS "SQLite Libraries not found")
else()
  add_compile_definitions(HAVE_SQLITE)
endif()

include(CheckTypeSize)
check_type_size(time_t SIZEOF_TIME)
if(NOT SIZEOF_TIME EQUAL 8)
  message(WARNING "Size of time_t is ${SIZEOF_TIME}")
endif()

# This is for the config.h file
include_directories(${PROJECT_BINARY_DIR}/src)
include_directories(src)
include_directories(src/lib/lua)

find_library(HAVE_READLINE NAMES readline)
if( NOT HAVE_READLINE )
    message(FATAL_ERROR "Readline Library not found!")
endif()
# message("Readline Libraries Found: ${HAVE_READLINE}")

find_library(HAVE_MQTT NAMES paho-mqtt3c)
if( NOT HAVE_MQTT )
  message("paho-mqtt3c library not found.  Not building MQTT client module.")
  set(BUILD_MQTT OFF)
endif()

find_library(HAVE_PLCTAG NAMES plctag)
if( NOT HAVE_PLCTAG )
  message("plctag library not found.  Not building daxplctag client module.")
  set(BUILD_PLCTAG OFF)
endif()

find_library(HAVE_OPEN62541 NAMES open62541)
if( NOT HAVE_OPEN62541 )
  message("OPEN62541 library not found.  Not building OPC UA module.")
  set(BUILD_OPCUA OFF)
endif()


add_subdirectory(src)

# Install the sample configurations
#file(GLOB files "etc/*.conf.sample")
#foreach(file ${files})
#  install(FILES ${file} DESTINATION etc/opendax)
#endforeach()


add_subdirectory(tests)


find_program (ASCIIDOCTOR_EXE asciidoctor)
if (NOT ASCIIDOCTOR_EXE)
    message (WARNING "Could not find asciidoctor: skipping docs")
    set (BUILD_DOC OFF)
else ()
    message (STATUS "Using asciidoctor at ${ASCIIDOCTOR_EXE}")
endif ()

if (BUILD_DOC)
    add_subdirectory(doc)
endif ()
