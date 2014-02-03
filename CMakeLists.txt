
project(libzling)
cmake_minimum_required(VERSION 2.8)

# source path
aux_source_directory(./src DIR_SRC)
aux_source_directory(./sample DIR_SAMPLE)

# include path
include_directories(./src)

# build dynamic library
add_library(
    zling SHARED ${DIR_SRC})

# build demo
add_executable(
    ./zling_demo ${DIR_SAMPLE})
target_link_libraries(./zling_demo zling)
