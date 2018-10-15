# (c) 2018 Christian Parpart <christian@parpart.family>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

cmake_minimum_required(VERSION 3.6)
project(actor VERSION "0.0.0" LANGUAGES CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

# ----------------------------------------------------------------------------
if(NOT MSVC)
  add_definitions(-Wall)
  add_definitions(-pedantic)
endif()

# ----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------
add_library(actor INTERFACE)
target_include_directories(actor INTERFACE include/)

if(NOT MSVC)
  target_link_libraries(actor INTERFACE pthread)
endif()

install(TARGETS actor DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/actor"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
    FILES_MATCHING PATTERN "*.h")

option(ACTOR_EXAMPLES "Build examples [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})

# ----------------------------------------------------------------------------
if(ACTOR_EXAMPLES)
  add_executable(actor-chain-demo examples/chain-demo.cpp)
  set_target_properties(actor-chain-demo PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(actor-chain-demo actor)

  add_executable(actor-time-server-demo examples/time-server-demo.cpp)
  set_target_properties(actor-time-server-demo PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(actor-time-server-demo actor)

  add_executable(logging-demo examples/logging-demo.cpp)
  set_target_properties(logging-demo PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(logging-demo actor)
endif(ACTOR_EXAMPLES)

# vim:ts=2:sw=2:et
