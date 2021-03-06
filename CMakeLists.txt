cmake_minimum_required(VERSION 2.6)
project(check_mqtt)
set(CHECK_MQTT_VERSION "1.0.0")
# set(CMAKE_BUILD_TYPE Debug)
 
set(DEBUG_BUILD 0)
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)
if (BUILD_TYPE STREQUAL DEBUG)
    set(DEBUG_BUILD 1)
endif(BUILD_TYPE STREQUAL DEBUG)

include (CheckFunctionExists)
include (CheckIncludeFile)
include (FindPkgConfig)

check_function_exists(memset HAVE_MEMSET)
check_function_exists(memcpy HAVE_MEMCPY)
check_function_exists(sigaction HAVE_SIGACTION)

check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
if (NOT HAVE_CLOCK_GETTIME)
    message(FATAL_ERROR "clock_gettime call is required")
endif(NOT HAVE_CLOCK_GETTIME)

check_include_file(stdbool.h HAVE_STDBOOL_H)

# check for libmosquitto
find_library(LIBMOSQUITTO mosquitto)
if (LIBMOSQUITTO-NOTFOUND)
    message(FATAL_ERROR "Required library libmosquitto not found")
endif(LIBMOSQUITTO-NOTFOUND)

# check for uuid
pkg_search_module(LIBUUID REQUIRED uuid)
include_directories(SYSTEM ${LIBUUID_INCLUDE_DIRS})
link_directories(${LIBUUID_LIBRARY_DIRS})

add_library(usage usage.c)
add_library(util util.c)
add_library(mqtt_functions mqtt_functions.c)
add_library(sig_handler sig_handler.c)

configure_file("${PROJECT_SOURCE_DIR}/check_mqtt.h.in" "${PROJECT_SOURCE_DIR}/check_mqtt.h")

add_executable(check_mqtt main.c)
target_link_libraries(check_mqtt usage)
target_link_libraries(check_mqtt util)
target_link_libraries(check_mqtt sig_handler)
target_link_libraries(check_mqtt mqtt_functions)
target_link_libraries(check_mqtt "-lmosquitto")
target_link_libraries(check_mqtt ${LIBUUID_LIBRARIES})
target_link_libraries(check_mqtt ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS check_mqtt DESTINATION lib/nagios/plugins)

