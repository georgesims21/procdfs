cmake_minimum_required(VERSION 3.10)
project(procsys 
    VERSION 1.0
    LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")
add_executable(procsys
        src/main.c
        src/server_new.c src/server_new.h
        src/ds_new.h src/ds_new.c
        src/pathnames.h src/fileops_new.c src/fileops_new.h)

target_compile_definitions(${PROJECT_NAME} PUBLIC USE_FUSE3 FUSE_USE_VERSION=35)
target_compile_options(procsys PUBLIC -Wall -Wextra -Wpedantic -g)
set(CMAKE_C_FLAGS "-D_FILE_OFFSET_BITS=64")

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBFUSE REQUIRED fuse3)
target_link_libraries(${PROJECT_NAME}
        ${LIBFUSE_LIBRARIES}
        ${LIBFUSE}
        fuse3
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

