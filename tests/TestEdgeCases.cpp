#include "TestOracles.hpp"

#include "cut/ApdfsSolver.hpp"
#include "graph/GraphBuilder.hpp"
#include "graph/GraphLoader.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace apdfs;

namespace
{

std::filesystem::path writeGraphFile(const std::string& name, const std::string& contents)
{
    auto dir = std::filesystem::path("outdir") / "edgecases";
    std::filesystem::create_directories(dir);
    auto path = dir / name;
    std::ofstream file(path);
    file << contents;
    return path;
}

std::vector<std::vector<EdgeIndex>> solve(const Graph& graph, SolverConfig config)
{
    config.collectInMemory = true;
    config.showProgress = false;
    std::filesystem::create_directories(config.outputDir);

    ApdfsSolver solver(graph, config);
    solver.run();
    return solver.getCuts();
}

Graph buildDiamond()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(3);
    builder.addEdges({{0, 1}, {0, 2}, {1, 3}, {2, 3}});
    return Graph(std::move(builder));
}

Graph buildRandom20x60()
{
    GraphBuilder builder;
    VertexIndex sink = 19;
    Xoroshiro128 rng(12345);

    for (VertexIndex i = 0; i < sink; ++i)
    {
        builder.addEdge(i, i + 1);
    }

    size_t remaining = 60 - static_cast<size_t>(sink);
    for (size_t i = 0; i < remaining; ++i)
    {
        auto u = static_cast<VertexIndex>(rng() % 20);
        auto v = static_cast<VertexIndex>(rng() % 20);
        if (u == v)
        {
            --i;
            continue;
        }
        if (rng() % 100 < 20 && v >= u)
        {
            std::swap(u, v);
        }
        builder.addEdge(u, v);
    }

    builder.setSuperSource(0);
    builder.setSuperSink(sink);
    return Graph(std::move(builder));
}

} // namespace

TEST(TestEdgeCases, Solver_ThreadCountZero_BehavesAsSingleThread)
{
    auto graph = buildDiamond();

    SolverConfig single;
    single.outputDir = "outdir/apdfs_edge_single";
    single.threadCount = 1;

    SolverConfig zero;
    zero.outputDir = "outdir/apdfs_edge_zero";
    zero.threadCount = 0;

    EXPECT_EQ(solve(graph, zero), solve(graph, single));
}

TEST(TestEdgeCases, Solver_MaxCutsStopsTheRun)
{
    auto graph = buildRandom20x60();

    SolverConfig unlimited;
    unlimited.outputDir = "outdir/apdfs_edge_unlimited";
    unlimited.threadCount = 1;
    const auto total = solve(graph, unlimited);
    ASSERT_GT(total.size(), 110U) << "the graph must produce enough cuts for this test to be meaningful";

    SolverConfig limited;
    limited.outputDir = "outdir/apdfs_edge_limited";
    limited.threadCount = 1;
    limited.maxCuts = 10;
    const auto cuts = solve(graph, limited);

    EXPECT_GE(cuts.size(), 10U) << "max-cuts must be reached";
    EXPECT_LE(cuts.size(), 110U) << "overshoot must stay within one batch";
    EXPECT_LT(cuts.size(), total.size()) << "the run must stop before exhausting the graph";
}

TEST(TestEdgeCases, Solver_CollectInMemoryFalse_WritesFilesOnly)
{
    auto graph = buildDiamond();
    const std::filesystem::path outputDir = "outdir/apdfs_edge_files_only";
    std::filesystem::remove_all(outputDir);

    SolverConfig config;
    config.outputDir = outputDir;
    config.threadCount = 1;
    config.collectInMemory = false;

    {
        ApdfsSolver solver(graph, config);
        solver.run();

        EXPECT_TRUE(solver.getCuts().empty()) << "collectInMemory=false must not retain cuts";

        size_t files = 0;
        uint64_t bytes = 0;
        for (const auto& entry : std::filesystem::directory_iterator(outputDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".bin")
            {
                ++files;
                bytes += std::filesystem::file_size(entry.path());
            }
        }
        EXPECT_GT(files, 0U);
        EXPECT_GT(bytes, 0U) << "flush() left the payload in the stdio buffer instead of the OS";
    }

    std::filesystem::remove_all(outputDir);
}

TEST(TestEdgeCases, Solver_SelfLoopNeverAppearsInACut)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(2);
    builder.addEdges({{0, 1}, {1, 2}, {1, 1}});
    auto graph = Graph(std::move(builder));

    SolverConfig config;
    config.outputDir = "outdir/apdfs_edge_selfloop";
    config.threadCount = 1;
    const auto cuts = solve(graph, config);

    ASSERT_EQ(cuts.size(), 2U);
    EXPECT_EQ(cuts[0], (std::vector<EdgeIndex>{0}));
    EXPECT_EQ(cuts[1], (std::vector<EdgeIndex>{1}));
}

TEST(TestEdgeCases, Loader_MalformedHeader_ReturnsNullopt)
{
    const auto path = writeGraphFile("malformed_header.txt", "not a header\n0 1\n");

    GraphLoader::Config config;
    config.filePath = path;
    config.superSource = 0;
    config.superSink = 1;

    EXPECT_FALSE(GraphLoader::loadFromFile(config).has_value());
    std::filesystem::remove_all(path.parent_path());
}

TEST(TestEdgeCases, Loader_TruncatedEdgeLine_ReturnsNullopt)
{
    const auto path = writeGraphFile("truncated_edge.txt", "3 3\n0 1\n0\n");

    GraphLoader::Config config;
    config.filePath = path;
    config.superSource = 0;
    config.superSink = 2;

    EXPECT_FALSE(GraphLoader::loadFromFile(config).has_value());
    std::filesystem::remove_all(path.parent_path());
}

TEST(TestEdgeCases, Loader_EmptyFile_ReturnsNullopt)
{
    const auto path = writeGraphFile("empty.txt", "");

    GraphLoader::Config loaderConfig;
    loaderConfig.filePath = path;
    loaderConfig.superSource = 0;
    loaderConfig.superSink = 1;

    EXPECT_FALSE(GraphLoader::loadFromFile(loaderConfig).has_value()) << "a file without a header is not a graph";
    std::filesystem::remove_all(path.parent_path());
}

TEST(TestEdgeCases, Solver_WritesEdgeMapMatchingGraph)
{
    auto graph = buildDiamond();
    const std::filesystem::path outputDir = "outdir/apdfs_edge_map";
    std::filesystem::remove_all(outputDir);

    {
        SolverConfig config;
        config.outputDir = outputDir;
        config.threadCount = 1;
        config.collectInMemory = false;

        ApdfsSolver solver(graph, config);
        solver.run();
    }

    std::ifstream mapFile(outputDir / "edges.map");
    ASSERT_TRUE(mapFile.is_open()) << "edges.map is what cutreader needs to decode cut ids";

    size_t entries = 0;
    std::string line;
    while (std::getline(mapFile, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        size_t index = 0;
        size_t source = 0;
        size_t target = 0;
        ASSERT_TRUE(static_cast<bool>(iss >> index >> source >> target)) << "bad line: " << line;
        ASSERT_LT(index, graph.edgeCount());

        const auto edge = static_cast<EdgeIndex>(index);
        EXPECT_EQ(source, graph.edgeSource(edge)) << line;
        EXPECT_EQ(target, graph.edgeTarget(edge)) << line;
        ++entries;
    }

    EXPECT_EQ(entries, graph.edgeCount());
    std::filesystem::remove_all(outputDir);
}

TEST(TestEdgeCases, Solver_BinOutputMatchesInMemoryCuts)
{
    auto graph = buildDiamond();
    const std::filesystem::path outputDir = "outdir/apdfs_edge_roundtrip";
    std::filesystem::remove_all(outputDir);

    std::vector<std::vector<EdgeIndex>> inMemory;
    {
        SolverConfig config;
        config.outputDir = outputDir;
        config.threadCount = 1;
        config.collectInMemory = true;

        ApdfsSolver solver(graph, config);
        solver.run();
        inMemory = solver.getCuts();
    }

    std::vector<std::vector<EdgeIndex>> fromDisk;
    for (const auto& path : test::listCutFiles(outputDir))
    {
        auto file = test::readCutFile(path);
        for (const auto& record : file.records)
        {
            std::vector<EdgeIndex> cut;
            cut.reserve(record.edges.size());
            for (uint32_t edge : record.edges)
            {
                cut.push_back(static_cast<EdgeIndex>(edge));
            }
            std::sort(cut.begin(), cut.end());
            fromDisk.push_back(std::move(cut));
        }
    }
    std::sort(fromDisk.begin(), fromDisk.end());

    EXPECT_FALSE(inMemory.empty());
    EXPECT_EQ(fromDisk, inMemory) << "what was streamed to .bin files must equal what the solver reports";

    std::filesystem::remove_all(outputDir);
}

TEST(TestEdgeCases, Solver_MaxCutSizeKeepsSmallCutsOnly)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(6);
    builder.addEdge(0, 1);
    for (VertexIndex v = 2; v <= 5; ++v)
    {
        builder.addEdge(1, v);
        builder.addEdge(v, 6);
    }
    auto graph = Graph(std::move(builder));

    auto solveWith = [&graph](size_t maxCutSize, uint64_t& filtered) {
        SolverConfig config;
        config.outputDir = "outdir/apdfs_edge_size";
        config.threadCount = 1;
        config.collectInMemory = true;
        config.maxCutSize = maxCutSize;

        std::filesystem::remove_all(config.outputDir);
        ApdfsSolver solver(graph, config);
        solver.run();
        filtered = solver.stats().cutsFiltered.load();
        return solver.getCuts();
    };

    uint64_t filteredAll = 0;
    const auto all = solveWith(0, filteredAll);
    ASSERT_EQ(all.size(), 17U) << "unlimited run must see 1 + 16 minimal cuts";
    EXPECT_EQ(filteredAll, 0U);

    uint64_t filteredOne = 0;
    const auto onlyOne = solveWith(1, filteredOne);
    ASSERT_EQ(onlyOne.size(), 1U);
    EXPECT_EQ(onlyOne.front().size(), 1U);
    EXPECT_EQ(filteredOne, 16U);

    uint64_t filteredTwo = 0;
    const auto onlyTwo = solveWith(2, filteredTwo);
    EXPECT_EQ(onlyTwo.size(), 1U) << "no cut of size 2 exists, the size-1 cut must survive";
    EXPECT_EQ(filteredTwo, 16U);

    uint64_t filteredFour = 0;
    const auto allFour = solveWith(4, filteredFour);
    EXPECT_EQ(allFour.size(), 17U);
    EXPECT_EQ(filteredFour, 0U);

    std::filesystem::remove_all("outdir/apdfs_edge_size");
}

TEST(TestEdgeCases, Solver_MaxExaminedCountsFilteredCuts)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(6);
    builder.addEdge(0, 1);
    for (VertexIndex v = 2; v <= 5; ++v)
    {
        builder.addEdge(1, v);
        builder.addEdge(v, 6);
    }
    auto graph = Graph(std::move(builder));

    SolverConfig config;
    config.outputDir = "outdir/apdfs_edge_size_and_count";
    config.threadCount = 1;
    config.collectInMemory = true;
    config.maxCutSize = 1;
    config.maxExamined = 3;
    std::filesystem::remove_all(config.outputDir);

    ApdfsSolver solver(graph, config);
    solver.run();

    const auto& stats = solver.stats();
    const uint64_t seen = stats.cutsFound.load() + stats.cutsFiltered.load();
    EXPECT_GE(seen, 3U) << "the limit must be reached even though almost every cut is filtered";
    EXPECT_LE(seen, 103U) << "overshoot stays within one 100-frame batch";
    EXPECT_LE(stats.cutsFound.load(), 1U) << "only one cut of size 1 exists";

    std::filesystem::remove_all(config.outputDir);
}

TEST(TestEdgeCases, Solver_MaxCutsCountsOnlyEmittedCuts)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(6);
    builder.addEdge(0, 1);
    for (VertexIndex v = 2; v <= 5; ++v)
    {
        builder.addEdge(1, v);
        builder.addEdge(v, 6);
    }
    auto graph = Graph(std::move(builder));

    SolverConfig config;
    config.outputDir = "outdir/apdfs_edge_emitted_limit";
    config.threadCount = 1;
    config.collectInMemory = true;
    config.maxCutSize = 1;
    config.maxCuts = 1;
    std::filesystem::remove_all(config.outputDir);

    ApdfsSolver solver(graph, config);
    solver.run();

    EXPECT_EQ(solver.stats().cutsFound.load(), 1U) << "the single size-1 cut triggers the limit";
    EXPECT_LE(solver.stats().cutsFiltered.load(), 100U) << "the run must stop promptly after emitting";

    std::filesystem::remove_all(config.outputDir);
}
