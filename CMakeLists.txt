cmake_minimum_required(VERSION 2.6)
project(gcruft2)

add_executable(gcruft2 main.c)

install(TARGETS gcruft2 RUNTIME DESTINATION bin)
