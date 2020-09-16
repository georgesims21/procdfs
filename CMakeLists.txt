cmake_minimum_required(VERSION 3.16)
project(procsys C)

set(CMAKE_C_STANDARD 11)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

#add_executable(procsys
#        src/main.c src/fileops.c
#        src/fileops.h src/client.c
#        src/client.h src/log.c
#        src/log.h src/writer.c
#        src/writer.h src/reader.c
#        src/reader.h src/queue.h
#        src/queue.c src/defs.h)

add_executable(server_new
        src/server_new.c src/server_new.h
        src/ds_new.c src/ds_new.h)
add_executable(procsys
        src/main.c
        src/log.h src/log.c
        src/server_new.c src/server_new.h
        src/ds_new.h src/ds_new.c)
add_executable(procsys2
        src/main.c src/fileops.c
        src/fileops.h src/client.c
        src/client.h src/log.c
        src/log.h src/writer.c
        src/writer.h src/reader.c
        src/reader.h src/queue.h
        src/queue.c src/defs.h
        src/server.c src/server.h)
add_executable(procsys3
        src/main.c src/fileops.c
        src/fileops.h src/client.c
        src/client.h src/log.c
        src/log.h src/writer.c
        src/writer.h src/reader.c
        src/reader.h src/queue.h
        src/queue.c src/defs.h
        src/server.c src/server.h)
#add_executable(procsys2
#        src/main.c src/fileops.c
#        src/fileops.h src/client.c
#        src/client.h src/log.c
#        src/log.h src/writer.c
#        src/writer.h src/reader.c
#        src/reader.h src/queue.h
#        src/queue.c src/defs.h)
add_executable(server
        src/server.c src/client-server.h
        src/server.h src/log.c
        src/log.h src/fileops.c
        src/fileops.h src/reader.c
        src/reader.h src/writer.c
        src/writer.h src/defs.h)
add_executable(client
        src/client.c src/client-server.h
        src/client.h src/log.c
        src/log.h src/fileops.c
        src/fileops.h src/reader.c
        src/reader.h src/writer.c
        src/writer.h src/queue.c
        src/queue.h src/defs.h)
add_executable(testing src/testing.c src/log.c src/log.h)

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
        m
        )
target_link_libraries(server_new
        pthread
        m
        )
