cmake_minimum_required(VERSION 3.10)
project(procdfs C)
#    VERSION 1.0
    #LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")
add_executable(procdfs
        src/main.c
        src/server.c src/server.h
        src/inprog.h src/inprog.c
        src/pathnames.h src/request.c src/request.h src/error.c src/error.h)

target_compile_definitions(${PROJECT_NAME} PUBLIC USE_FUSE FUSE_USE_VERSION=35)
#include_directories(SYSTEM ${MPI_INCLUDE_PATH})

target_compile_options(procdfs PUBLIC -Wall -Wextra -Wpedantic -g -std=c11)
set(CMAKE_C_FLAGS "-D_FILE_OFFSET_BITS=64")

#find_package(MPI REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(LIBFUSE REQUIRED fuse)
target_link_libraries(${PROJECT_NAME}
        ${LIBFUSE_LIBRARIES}
        ${LIBFUSE}
	#        ${MPI_C_LIBRARIES}
        pthread
        m
        )



#https://github.com/thejinx0r/DriveFS/blob/master/CMakeLists.txt
#lib fuse3
#if(USE_FUSE3)
#    pkg_search_module(LIBFUSE REQUIRED fuse3)
#    target_compile_definitions(${PROJECT_NAME} PUBLIC USE_FUSE3
#            FUSE_USE_VERSION=35)
#
#else()
#    pkg_search_module(LIBFUSE REQUIRED fuse)
#    target_compile_definitions(${PROJECT_NAME} PUBLIC
#            FUSE_USE_VERSION=29)
#endif()

