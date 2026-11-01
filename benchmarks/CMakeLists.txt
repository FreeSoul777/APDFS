add_executable(bench_apdfs bench_apdfs.cpp)
target_link_libraries(bench_apdfs PRIVATE apdfs_cut benchmark::benchmark)
