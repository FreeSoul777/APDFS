add_executable(apdfs_tests
    TestMain.cpp
    TestCli.cpp
    TestTypes.cpp
    TestXoroshiro128.cpp
    TestBitset.cpp
    TestSpinLock.cpp
    TestGraph.cpp
    TestGraphLoader.cpp
    TestSeenTable.cpp
    TestBfsEngine.cpp
    TestCutProcessor.cpp
    TestApdfsSolver.cpp
    TestCompleteness.cpp
    TestCutWriter.cpp
    TestSpillBuffer.cpp
    TestEdgeCases.cpp
    TestPerformance.cpp
    TestIntegration.cpp
)
target_link_libraries(apdfs_tests PRIVATE
    apdfs_cut
    apdfs_app
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(apdfs_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(apdfs_tests
    PROPERTIES
        TIMEOUT 60
        LABELS "unit"
)