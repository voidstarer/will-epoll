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

#IF(APPLE)
#   INCLUDE_DIRECTORIES ( /usr/local/include/openssl )
#   FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
#   MARK_AS_ADVANCED ( OpenGL_LIBRARY)
#   SET(EXTRA_GL_LIBS ${COCOA_LIBRARY} ${GLUT_LIBRARY} ${OpenGL_LIBRARY})
#ENDIF (APPLE)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)


add_library(networking STATIC IMPORTED)
set_property(TARGET networking PROPERTY IMPORTED_LOCATION "../lib/networking/lib/libnetworking.a")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/networking/include)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(client
    client.cpp
)

add_executable(server
    server.cpp
)

#IF(APPLE)
#add_executable(OpenGL
#	OpenGL.cpp
#)
#ENDIF(APPLE)


target_link_libraries(client networking crypto pthread)
target_link_libraries(server networking crypto pthread)

#IF(APPLE)
#  target_link_libraries(OpenGL networking crypto pthread ${EXTRA_GL_LIBS})
#ENDIF(APPLE)

#target_link_libraries(PiTest networking crypto pthread)
#target_link_libraries(PiRotationTest networking crypto pthread)
