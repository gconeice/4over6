cmake_minimum_required(VERSION 3.7)
project(4over6_modern)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS "-Wall")

file(GLOB SOURCE_FILES ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h)

add_executable(4over6_modern ${SOURCE_FILES})

target_link_libraries(4over6_modern pthread)