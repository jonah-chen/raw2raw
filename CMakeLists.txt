cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(raw2raw LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(LIBRAW_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LibRaw/" CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

find_package(OpenMP REQUIRED)
find_package(OpenGL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -m64")

# print cmake release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/frontend/glfw/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/frontend/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/frontend/imgui/backends)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/frontend/glad/include)

add_subdirectory(LibRaw-cmake)

add_library(raw2raw STATIC core/profile.cc
        core/parse.cc
        core/p_reduce.cc
        core/reduce_extra.cc
)
target_link_options(raw2raw PUBLIC -static)
target_link_libraries(raw2raw PUBLIC libraw::libraw_r OpenMP::OpenMP_CXX)


add_executable(raw2rawcli raw2rawcli.cc)
target_link_options(raw2rawcli PRIVATE -static)
target_link_libraries(raw2rawcli PRIVATE raw2raw)

add_subdirectory(frontend/glfw)

add_library(imgui STATIC frontend/imgui/imgui.cpp
        frontend/imgui/imgui_draw.cpp
        frontend/imgui/imgui_widgets.cpp
        frontend/imgui/imgui_tables.cpp
        frontend/imgui/imgui_demo.cpp
        frontend/imgui/backends/imgui_impl_glfw.cpp
        frontend/imgui/backends/imgui_impl_opengl3.cpp
)
target_link_options(imgui PUBLIC -static)
target_link_libraries(imgui PUBLIC glfw ${OPENGL_gl_LIBRARY})

add_executable(raw2rawapp raw2rawapp.cc
        frontend/glad/src/glad.cc
        frontend/loupe.cc
        frontend/thumbnail.cc
        frontend/state_primary.cc
        frontend/state_filetree.cc
        frontend/state_infos.cc
        frontend/state_grid_view.cc
)
target_link_options(raw2rawapp PRIVATE -static)
target_link_libraries(raw2rawapp PRIVATE raw2raw imgui)
