cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(actor VERSION "0.0.0" LANGUAGES CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

include(ClangTidy)
include(PedanticCompiler)

# ----------------------------------------------------------------------------
# download CPM.cmake
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# ----------------------------------------------------------------------------
if(NOT MSVC)
  add_definitions(-Wall)
  add_definitions(-pedantic)
endif()

# ----------------------------------------------------------------------------
add_library(actor INTERFACE)
target_include_directories(actor INTERFACE include/)
target_compile_features(actor INTERFACE cxx_std_23)

if(NOT MSVC)
  target_link_libraries(actor INTERFACE pthread)
endif()

install(TARGETS actor DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/actor"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
    FILES_MATCHING PATTERN "*.hpp")


# ----------------------------------------------------------------------------
option(ACTOR_EXAMPLES "Build Actor examples [default: ${MASTER_PROJECT}]" ${MASTER_PROJECT})

if(ACTOR_EXAMPLES)
  if(ACTOR_EXAMPLES)
    CPMAddPackage("gh:contour-terminal/reflection-cpp#master")
  endif()

  add_executable(channel-demo examples/channel-demo.cpp)
  set_target_properties(channel-demo PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(channel-demo actor reflection-cpp::reflection-cpp)

  add_executable(channel-demo2 examples/channel-demo2.cpp)
  set_target_properties(channel-demo2 PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(channel-demo2 actor reflection-cpp::reflection-cpp)

  add_executable(channel-demo3 examples/channel-demo3.cpp)
  set_target_properties(channel-demo3 PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(channel-demo3 actor reflection-cpp::reflection-cpp)

  add_executable(channel-demo4 examples/channel-demo4.cpp)
  set_target_properties(channel-demo4 PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(channel-demo4 actor reflection-cpp::reflection-cpp)

  add_executable(chain-demo examples/chain-demo.cpp)
  set_target_properties(chain-demo PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(chain-demo actor)

  add_executable(time-server-demo examples/time-server-demo.cpp)
  set_target_properties(time-server-demo PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(time-server-demo actor)

  add_executable(logging-demo examples/logging-demo.cpp)
  set_target_properties(logging-demo PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(logging-demo actor)
endif(ACTOR_EXAMPLES)

# vim:ts=2:sw=2:et
