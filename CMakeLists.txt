
project(libzling)
cmake_minimum_required(VERSION 2.8)

# source path
aux_source_directory("./src" DIR_SRC)
aux_source_directory("./sample" DIR_SAMPLE)

# include path
include_directories("./src")

# build dynamic library
add_library(
    zling SHARED ${DIR_SRC})

# build demo
add_executable(
    zling_demo ${DIR_SAMPLE})
target_link_libraries(zling_demo zling)

# install
install(FILES "src/libzling.h"       DESTINATION "./include")
install(FILES "src/libzling_utils.h" DESTINATION "./include")
install(FILES "src/libzling_inc.h"   DESTINATION "./include")
install(TARGETS zling                DESTINATION "./lib")
install(TARGETS zling_demo           DESTINATION "./bin")
