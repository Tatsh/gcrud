cmake_minimum_required(VERSION 2.8)
project(gcrud)

add_compile_options(-fno-builtin -std=gnu11)
set(CMAKE_C_FLAGS_DEBUG "-ggdb \
-O0 \
-Wall \
-Wdeprecated-declarations \
-Wempty-body \
-Werror \
-Werror=conversion \
-Werror=float-conversion \
-Werror=implicit-function-declaration \
-Werror=incompatible-pointer-types \
-Werror=int-conversion \
-Werror=return-type \
-Werror=sign-conversion \
-Wmissing-braces \
-Wmissing-field-initializers \
-Wno-missing-prototypes \
-Wno-strict-prototypes \
-Wno-trigraphs \
-Wno-unknown-pragmas \
-Wparentheses \
-Wpointer-sign \
-Wshadow \
-Wsign-compare \
-Wswitch \
-Wuninitialized \
-Wunknown-pragmas \
-Wunreachable-code \
-Wunused-function \
-Wunused-label \
-Wunused-parameter \
-Wunused-value \
-Wunused-variable \
-Wwrite-strings")
add_compile_options(
  $<$<AND:$<C_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-pedantic>
  $<$<AND:$<C_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Wno-gnu-empty-struct>
  $<$<AND:$<C_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Wno-language-extension-token>
  )

if(NOT CMAKE_MODULE_PATH)
  set(CMAKE_MODULE_PATH)
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/helper/")
find_package(GLIB2 REQUIRED)

add_executable(gcrud colors.c util.c whitelist.c main.c)
target_include_directories(gcrud PRIVATE ${GLIB2_INCLUDE_DIRS})
target_link_libraries(gcrud ${GLIB2_LIBRARIES})

install(TARGETS gcrud RUNTIME DESTINATION bin)
