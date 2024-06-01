cmake_minimum_required(VERSION 3.29 FATAL_ERROR)

project(raw2raw LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

set(LIBRAW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LibRaw/" CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

find_package(OpenMP REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(LibRaw-cmake)


add_library(raw2raw STATIC core/profile.cc core/parse.cc core/reduce.cc core/reduce_extra.cc core/summary.cc)
target_link_libraries(raw2raw PRIVATE libraw::libraw_r OpenMP::OpenMP_CXX ${PERL_LIBRARIES})


add_executable(raw2rawcli raw2rawcli.cc)
target_link_libraries(raw2rawcli PRIVATE raw2raw)
