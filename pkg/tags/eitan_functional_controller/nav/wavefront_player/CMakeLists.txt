cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(wavefront_player)
rospack_add_executable(wavefront_player wavefront_player.cc)
target_link_libraries(wavefront_player wavefront_standalone)

# this little program just lets you send and block on goals from the shell
rospack_add_executable(cli cli.cpp)
