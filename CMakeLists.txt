cmake_minimum_required(VERSION 3.16)
project(procsys C)

set(CMAKE_C_STANDARD 11)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")
add_executable(procsys src/main.c src/fileops.c src/fileops.h src/client.c src/client.h src/log.c src/log.h)
#add_executable(procsys test/net-tools-code/arp.c)

add_executable(server src/server.c src/client-server.h src/server.h src/log.c src/log.h)
add_executable(client src/client.c src/client-server.h src/client.h src/log.c src/log.h)
set(CMAKE_C_FLAGS "-D_FILE_OFFSET_BITS=64 -lfuse3")

#https://github.com/thejinx0r/DriveFS/blob/master/CMakeLists.txt
#lib fuse3
find_package(PkgConfig REQUIRED)

if(USE_FUSE3)
    pkg_search_module(LIBFUSE REQUIRED fuse3)
    target_compile_definitions(${PROJECT_NAME} PUBLIC USE_FUSE3
            FUSE_USE_VERSION=35)

else()
    pkg_search_module(LIBFUSE REQUIRED fuse)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
            FUSE_USE_VERSION=29)
endif()

target_link_libraries(${PROJECT_NAME}
        ${LIBFUSE_LIBRARIES}
        pthread
        )
