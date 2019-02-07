cmake_minimum_required(VERSION 2.6)
project(check_mqtt)
set(CHECK_MQTT_VERSION "1.0.0")
# set(CMAKE_BUILD_TYPE Debug)

include (CheckFunctionExists)
include (FindPkgConfig)

check_function_exists(memset HAVE_MEMSET)

# check for libmosquitto
find_library(LIBMOSQUITTO mosquitto)
if (LIBMOSQUITTO-NOTFOUND)
    message(FATAL_ERROR "Required library libmosquitto not found")
endif(LIBMOSQUITTO-NOTFOUND)

# check for uuid
pkg_search_module(LIBUUID REQUIRED uuid)
include_directories(SYSTEM ${LIBUUID_INCLUDE_DIRS})
link_directories(${LIBUUID_LIBRARY_DIRS})

add_library(handler handler.c)
add_library(mqtt mqtt.c)
add_library(usage usage.c)
add_library(util util.c)

configure_file("${PROJECT_SOURCE_DIR}/check_mqtt.h.in" "${PROJECT_SOURCE_DIR}/check_mqtt.h")

add_executable(check_mqtt main.c)
target_link_libraries(check_mqtt handler)
target_link_libraries(check_mqtt usage)
target_link_libraries(check_mqtt util)
target_link_libraries(check_mqtt mqtt)
target_link_libraries(check_mqtt "-lmosquitto")
target_link_libraries(check_mqtt ${LIBUUID_LIBRARIES})
target_link_libraries(check_mqtt ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS check_mqtt DESTINATION lib/nagios/plugins)

