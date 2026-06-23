cmake_minimum_required(VERSION 3.14)
project(chat C)

set(CMAKE_C_STANDARD 11)

if(NOT DEFINED CHAT_BUILD_VERSION)
    set(CHAT_BUILD_VERSION 0)
endif()

option(CHAT_NATIVE "Enable native hardware optimizations" OFF)

if(NOT MSVC)
    add_compile_options(-O3 -Wall -Wextra -Werror)
    if(CHAT_NATIVE)
        add_compile_options(-march=native)
    endif()
    add_definitions(-D_GNU_SOURCE)
else()
    add_compile_options(/O2 /W3 /WX)
endif()

set_source_files_properties(src/libchat.c PROPERTIES
    COMPILE_DEFINITIONS KC_CHAT_BUILD_VERSION=${CHAT_BUILD_VERSION}
)

# Main executable
add_executable(chat
    src/chat.c
    src/libchat.c
)

target_include_directories(chat PRIVATE src)

if(UNIX AND NOT ANDROID)
    target_link_libraries(chat pthread m)
endif()

# Static library
add_library(chat_static STATIC src/libchat.c)

set_target_properties(chat_static PROPERTIES OUTPUT_NAME "chat")

target_include_directories(chat_static PRIVATE src)

# Shared library
add_library(chat_shared SHARED src/libchat.c)

set_target_properties(chat_shared PROPERTIES OUTPUT_NAME "chat")

target_include_directories(chat_shared PRIVATE src)

if(UNIX AND NOT ANDROID)
    target_link_libraries(chat_shared pthread m)
endif()
