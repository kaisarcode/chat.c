cmake_minimum_required(VERSION 3.14)
project(chat C)

set(CMAKE_C_STANDARD 11)

if(NOT DEFINED CHAT_BUILD_VERSION)
    set(CHAT_BUILD_VERSION 0)
endif()

option(CHAT_NATIVE "Enable native hardware optimizations" OFF)
option(CHAT_BUILD_TESTS "Build chat tests" OFF)

set(CHAT_TEST_EXTERNAL_ARTIFACTS OFF)
if(CHAT_BUILD_TESTS AND CHAT_TEST_SHARED_LIBRARY)
    set(CHAT_TEST_EXTERNAL_ARTIFACTS ON)
endif()

if(NOT MSVC)
    add_compile_options(-O3 -Wall -Wextra -Werror)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        add_compile_options(-Wno-unused-command-line-argument)
    endif()
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

if(NOT CHAT_TEST_EXTERNAL_ARTIFACTS)
    # Main executable
    add_executable(chat
        src/chat.c
        src/libchat.c
    )

    target_include_directories(chat PRIVATE src)

    if(UNIX AND NOT ANDROID AND NOT APPLE)
        target_link_libraries(chat pthread m)
    endif()

    # Static library
    add_library(chat_static STATIC src/libchat.c)

    set_target_properties(chat_static PROPERTIES OUTPUT_NAME "chat")

    target_include_directories(chat_static PRIVATE src)

    # Shared library
    add_library(chat_shared SHARED src/libchat.c)

    set_target_properties(chat_shared PROPERTIES OUTPUT_NAME "chat")
    set_target_properties(chat_shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

    target_include_directories(chat_shared PRIVATE src)

    if(UNIX AND NOT ANDROID AND NOT APPLE)
        target_link_libraries(chat_shared pthread m)
    endif()
endif()

if(CHAT_BUILD_TESTS)
    enable_testing()

    if(CHAT_TEST_SHARED_LIBRARY)
        add_library(chat_test_shared SHARED IMPORTED)
        set_target_properties(chat_test_shared PROPERTIES
            IMPORTED_LOCATION "${CHAT_TEST_SHARED_LIBRARY}"
        )
        if(CHAT_TEST_IMPORT_LIBRARY)
            set_target_properties(chat_test_shared PROPERTIES
                IMPORTED_IMPLIB "${CHAT_TEST_IMPORT_LIBRARY}"
            )
        endif()
    else()
        set(CHAT_TEST_SHARED_TARGET chat_shared)
    endif()

    if(TARGET chat_test_shared)
        set(CHAT_TEST_SHARED_TARGET chat_test_shared)
    endif()

    add_executable(chat_contract_test src/test.c)
    target_include_directories(chat_contract_test PRIVATE src)
    target_link_libraries(chat_contract_test PRIVATE ${CHAT_TEST_SHARED_TARGET})

    add_custom_command(TARGET chat_contract_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${CHAT_TEST_SHARED_TARGET}>
            $<TARGET_FILE_DIR:chat_contract_test>
    )

    add_test(NAME library-kc_chat_open COMMAND chat_contract_test kc_chat_open)
    add_test(NAME library-kc_chat_close COMMAND chat_contract_test kc_chat_close)
    add_test(NAME library-kc_chat_exec COMMAND chat_contract_test kc_chat_exec)
    add_test(NAME library-kc_chat_is_exit COMMAND chat_contract_test kc_chat_is_exit)
    add_test(NAME library-kc_chat_free COMMAND chat_contract_test kc_chat_free)
    add_test(NAME library-kc_chat_options_default COMMAND chat_contract_test kc_chat_options_default)
    add_test(NAME library-kc_chat_options_load_env COMMAND chat_contract_test kc_chat_options_load_env)
    add_test(NAME library-kc_chat_options_free COMMAND chat_contract_test kc_chat_options_free)
    add_test(NAME library-kc_chat_on_signal COMMAND chat_contract_test kc_chat_on_signal)
    add_test(NAME library-kc_chat_raise_signal COMMAND chat_contract_test kc_chat_raise_signal)
    add_test(NAME library-kc_chat_listen_signals COMMAND chat_contract_test kc_chat_listen_signals)
    add_test(NAME library-kc_chat_listen_signal COMMAND chat_contract_test kc_chat_listen_signal)
    add_test(NAME library-kc_chat_signal_listener COMMAND chat_contract_test kc_chat_signal_listener)
    add_test(NAME library-kc_chat_stop COMMAND chat_contract_test kc_chat_stop)
    add_test(NAME library-kc_chat_version COMMAND chat_contract_test kc_chat_version)
endif()
