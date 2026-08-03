function(configure_apdfs_target target)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${target} PRIVATE 
            -g -O0 -fno-omit-frame-pointer
            -Wall -Wextra -Wpedantic -Werror
        )
        target_compile_definitions(${target} PRIVATE APDFS_DEBUG)
    else()
        target_compile_options(${target} PRIVATE 
            -O3 -DNDEBUG
            -Wall -Wextra -Wpedantic -Werror
        )
    endif()
    
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wdouble-promotion
            -Wformat=2
            -Wundef
            -Wunused-parameter
        )
    endif()
endfunction()