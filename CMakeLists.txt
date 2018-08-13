cmake_minimum_required(VERSION 2.6)
project(gcruft2)

find_library(
    USTR_LIB
    ustr
)
if(NOT USTR_LIB)
    message(FATAL_ERROR "Need ustr")
endif()


include_directories(${CMAKE_SOURCE_DIR}/pbl/src/src)
add_executable(gcruft2 main.c)
target_link_libraries(gcruft2 ${USTR_LIB})
# Temporary
target_link_libraries(gcruft2 ${CMAKE_SOURCE_DIR}/pbl/src/src/libpbl.a)

install(TARGETS gcruft2 RUNTIME DESTINATION bin)
