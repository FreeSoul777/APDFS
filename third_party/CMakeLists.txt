find_path(CONCURRENTQUEUE_INCLUDE_DIR
    NAMES concurrentqueue.h
    PATH_SUFFIXES concurrentqueue
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/concurrentqueue
    NO_DEFAULT_PATH
)

if(NOT CONCURRENTQUEUE_INCLUDE_DIR)
    message(FATAL_ERROR
        "concurrentqueue submodule not found!\n"
        "Run: git submodule update --init --recursive"
    )
endif()

add_library(concurrentqueue INTERFACE)
target_include_directories(concurrentqueue INTERFACE ${CONCURRENTQUEUE_INCLUDE_DIR})