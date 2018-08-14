cmake_minimum_required(VERSION 2.8)
project(gcruft2)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -ggdb -Wall -O0")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/helper/")
find_package(GLIB2 REQUIRED)

add_executable(gcruft2 main.c)
target_include_directories(gcruft2 PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(gcruft2 ${GLIB_LIBRARIES})

install(TARGETS gcruft2 RUNTIME DESTINATION bin)
