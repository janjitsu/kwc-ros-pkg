cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(nav_view_sdl)
rospack_add_executable(nav_view nav_view.cpp)
target_link_libraries(nav_view SDL_image)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

