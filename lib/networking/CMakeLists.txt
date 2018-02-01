cmake_minimum_required(VERSION 2.8)


set(CPACK_CMAKE_GENERATORS "Unix Makefiles")


add_definitions("-Wall")
add_definitions("-W")
add_definitions("-Wpedantic")
add_definitions("-Wextra")
add_definitions("-Wunreachable-code")
add_definitions("-Wfatal-errors")
add_definitions("-Wno-missing-field-initializers")
add_definitions("-Wwrite-strings")
add_definitions("-Wno-deprecated")
add_definitions("-Wno-unused")
add_definitions("-Wno-non-virtual-dtor")
add_definitions("-Wno-variadic-macros")
add_definitions("--std=c++11")
add_definitions("-g")



#find_package(OpenSSL REQUIRED)
#include_directories(${OPENSSL_INCLUDE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)



include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(networking STATIC
    src/io/event_io.cpp
    src/io/network_io.cpp
    src/io/tcp_client.cpp
    src/io/tcp_server.cpp
)



