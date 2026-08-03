cmake_minimum_required(VERSION 3.20)
project(apdfs VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

option(APDFS_BUILD_TESTS "Build unit tests" ON)
option(APDFS_BUILD_BENCHMARKS "Build benchmarks" OFF) 
option(APDFS_BUILD_TOOLS "Build tools" ON)
option(APDFS_ENABLE_CLANG_TIDY "Enable clang-tidy target" OFF)
option(APDFS_ENABLE_CLANG_FORMAT "Enable clang-format targets" ON)

include(cmake/CompileOptions.cmake)

add_subdirectory(third_party)
add_subdirectory(src)

include(cmake/Dependencies.cmake)
apdfs_fetch_dependencies()

if(APDFS_ENABLE_CLANG_FORMAT)
    find_program(CLANG_FORMAT_EXE NAMES clang-format REQUIRED)

    execute_process(
        COMMAND git ls-files "*.cpp" "*.hpp" "*.h"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE ALL_SOURCE_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REPLACE "\n" ";" ALL_SOURCE_FILES "${ALL_SOURCE_FILES}")
    
    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on all sources"
        VERBATIM
    )
    add_custom_target(clang-format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking code formatting"
        VERBATIM
    )
endif()

if(APDFS_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
    
    execute_process(
        COMMAND git ls-files "*.cpp" "*.hpp"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE ALL_CPP_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REPLACE "\n" ";" ALL_CPP_FILES "${ALL_CPP_FILES}")
    
    add_custom_target(clang-tidy-check
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR}
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            --header-filter="${CMAKE_SOURCE_DIR}/src/.*"
            ${ALL_CPP_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking with clang-tidy"
        VERBATIM
    )
    add_custom_target(clang-tidy-fix
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} --fix
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            --header-filter="${CMAKE_SOURCE_DIR}/src/.*"
            ${ALL_CPP_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Fixing with clang-tidy"
        VERBATIM
    )
endif()

if(APDFS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(APDFS_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(APDFS_BUILD_TOOLS)
    add_subdirectory(tools)
endif()