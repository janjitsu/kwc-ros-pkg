cmake_minimum_required(VERSION 2.6)
include(rosbuild)
rospack(katana)
gensrv()
add_subdirectory(libkatana)
add_subdirectory(standalone)
add_subdirectory(nodes)

