add_executable(graphgen graphgen.cpp)
target_link_libraries(graphgen PRIVATE apdfs_core)

add_executable(cutreader cutreader.cpp)
target_link_libraries(cutreader PRIVATE apdfs_core)
