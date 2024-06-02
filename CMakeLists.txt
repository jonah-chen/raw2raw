cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(raw2raw LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

set(LIBRAW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LibRaw/" CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

find_package(OpenMP REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(LibRaw-cmake)

add_library(raw2raw STATIC core/profile.cc core/parse.cc core/reduce.cc core/reduce_extra.cc core/summary.cc)
target_link_options(raw2raw PUBLIC -static)
target_link_libraries(raw2raw PUBLIC libraw::libraw_r OpenMP::OpenMP_CXX)


add_executable(raw2rawcli raw2rawcli.cc)
target_link_options(raw2rawcli PRIVATE -static)
target_link_libraries(raw2rawcli PRIVATE raw2raw)
