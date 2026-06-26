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
    set_target_properties(chat_shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

    target_include_directories(chat_shared PRIVATE src)

    if(UNIX AND NOT ANDROID)
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

    add_test(NAME library-options COMMAND chat_contract_test options)
    add_test(NAME library-context COMMAND chat_contract_test context)
    add_test(NAME library-exit COMMAND chat_contract_test exit)
    add_test(NAME library-exec COMMAND chat_contract_test exec)
    add_test(NAME library-signal COMMAND chat_contract_test signal)
    if(NOT WIN32)
        add_test(NAME library-multictx-stop COMMAND chat_contract_test multictx-stop)
    endif()

    if(CHAT_TEST_CLI)
        add_executable(chat_test_cli IMPORTED)
        set_target_properties(chat_test_cli PROPERTIES
            IMPORTED_LOCATION "${CHAT_TEST_CLI}"
        )
        set(CHAT_TEST_CLI_COMMAND chat_test_cli)
    else()
        set(CHAT_TEST_CLI_COMMAND chat)
    endif()

    add_test(NAME cli-help COMMAND ${CHAT_TEST_CLI_COMMAND} --help)
    add_test(NAME cli-version COMMAND ${CHAT_TEST_CLI_COMMAND} --version)
    add_test(NAME cli-unknown COMMAND ${CHAT_TEST_CLI_COMMAND} --unknown)
    set_tests_properties(cli-unknown PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli-missing-command COMMAND ${CHAT_TEST_CLI_COMMAND})
    set_tests_properties(cli-missing-command PROPERTIES WILL_FAIL TRUE)
endif()
