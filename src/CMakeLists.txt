add_library(apdfs_core STATIC
    core/Shutdown.cpp
)
target_include_directories(apdfs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apdfs_core PUBLIC Threads::Threads)
configure_apdfs_target(apdfs_core)

add_library(apdfs_graph STATIC
    graph/Graph.cpp
    graph/GraphBuilder.cpp
    graph/GraphLoader.cpp
)
target_include_directories(apdfs_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apdfs_graph PUBLIC apdfs_core)
configure_apdfs_target(apdfs_graph)

add_library(apdfs_cut STATIC
    cut/SeenTable.cpp
    cut/BfsEngine.cpp
    cut/CutProcessor.cpp
    cut/CutWriter.cpp
    cut/ApdfsSolver.cpp
)
target_include_directories(apdfs_cut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apdfs_cut PUBLIC apdfs_graph concurrentqueue)
configure_apdfs_target(apdfs_cut)

add_executable(apdfs_cli app/main.cpp)
target_link_libraries(apdfs_cli PRIVATE apdfs_cut)
configure_apdfs_target(apdfs_cli)