#include "TestOracles.hpp"

#include "cut/ApdfsSolver.hpp"
#include "graph/GraphBuilder.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <set>
#include <vector>

using namespace apdfs;

namespace
{

Graph buildSingleEdge()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(1);
    builder.addEdge(0, 1);
    return Graph(std::move(builder));
}

Graph buildPath3()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(2);
    builder.addEdge(0, 1);
    builder.addEdge(1, 2);
    return Graph(std::move(builder));
}

Graph buildDiamond()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(3);
    builder.addEdges({{0, 1}, {0, 2}, {1, 3}, {2, 3}});
    return Graph(std::move(builder));
}

Graph buildSpecExample()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(5);
    builder.addEdges({{0, 1}, {1, 2}, {1, 3}, {1, 4}, {2, 3}, {2, 4}, {3, 4}, {4, 5}});
    return Graph(std::move(builder));
}

Graph buildIsolatedVertexGraph()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(2);
    builder.addEdge(0, 1);
    builder.addEdge(1, 2);
    return Graph(std::move(builder));
}

std::vector<std::vector<EdgeIndex>> findAllCuts(const Graph& g, size_t threads = 1)
{
    SolverConfig config;
    config.outputDir = "outdir/apdfs_test";
    config.threadCount = threads;
    config.collectInMemory = true;
    config.showProgress = false;

    std::filesystem::create_directories(config.outputDir);

    ApdfsSolver solver(g, config);
    solver.run();
    return solver.getCuts();
}

} // namespace

TEST(TestApdfsSolver, A_001_SingleEdge)
{
    auto g = buildSingleEdge();
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 1U);
    ASSERT_EQ(result[0].size(), 1U);
    EXPECT_EQ(result[0][0], 0U);
}

TEST(TestApdfsSolver, A_002_PathLength2)
{
    auto g = buildPath3();
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 2U);
    std::vector<std::vector<EdgeIndex>> expected = {{0}, {1}};
    EXPECT_EQ(result, expected);
}

TEST(TestApdfsSolver, A_003_Diamond)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 4U);
    std::vector<std::vector<EdgeIndex>> expected = {{0, 1}, {0, 3}, {1, 2}, {2, 3}};
    EXPECT_EQ(result, expected);
}

TEST(TestApdfsSolver, A_004_ChainOfDiamonds_K2)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(6);
    builder.addEdges({{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}, {3, 5}, {4, 6}, {5, 6}});
    auto g = Graph(std::move(builder));
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 8U);
    for (const auto& cut : result)
    {
        EXPECT_EQ(cut.size(), 2U);
    }
}

TEST(TestApdfsSolver, A_005_SpecExample)
{
    auto g = buildSpecExample();
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 6U);
    for (const auto& cut : result)
    {
        EXPECT_FALSE(cut.empty());
    }
}

TEST(TestApdfsSolver, B_001_AllCutsAreValid)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    for (const auto& cut : result)
    {
        DynamicBitset forbidden(g.edgeCount());
        for (auto e : cut)
        {
            forbidden.set(e);
        }

        BfsEngine bfs(g.vertexCount(), g.edgeCount());
        std::vector<EdgeIndex> dummy;
        bool reachedSink = bfs.forwardBfs(g.superSource(), g.superSink(), g, forbidden, dummy);

        EXPECT_FALSE(reachedSink) << "Cut does not separate S* and T*";
    }
}

TEST(TestApdfsSolver, B_002_MinCutCardinality)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    for (const auto& cut : result)
    {
        EXPECT_EQ(cut.size(), 2U) << "All min cuts in diamond must have cardinality 2";
    }
}

TEST(TestApdfsSolver, B_003_NoDuplicates)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    std::set<std::vector<EdgeIndex>> unique(result.begin(), result.end());
    EXPECT_EQ(unique.size(), result.size()) << "Duplicate cuts found in result";
}

TEST(TestApdfsSolver, B_004_MinCutEqualsMaxFlow)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    size_t maxFlow = test::maxFlowEdmondsKarp(g, g.superSource(), g.superSink());
    EXPECT_EQ(maxFlow, 2U);

    for (const auto& cut : result)
    {
        EXPECT_EQ(cut.size(), maxFlow) << "Cut cardinality must equal max-flow";
    }
}

TEST(TestApdfsSolver, B_005_EachEdgeHasFreePath)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    for (const auto& cut : result)
    {
        for (EdgeIndex ei : cut)
        {
            DynamicBitset forbidden(g.edgeCount());
            for (EdgeIndex e : cut)
            {
                if (e != ei)
                {
                    forbidden.set(e);
                }
            }

            BfsEngine bfs(g.vertexCount(), g.edgeCount());
            std::vector<EdgeIndex> dummy;
            bool reachedSink = bfs.forwardBfs(g.superSource(), g.superSink(), g, forbidden, dummy);

            EXPECT_TRUE(reachedSink) << "Edge " << ei << " does not have a free path";
        }
    }
}

TEST(TestApdfsSolver, B_006_SourceTargetCorrect)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    for (const auto& cut : result)
    {
        DynamicBitset forbidden(g.edgeCount());
        for (auto e : cut)
        {
            forbidden.set(e);
        }

        BfsEngine bfs(g.vertexCount(), g.edgeCount());
        std::vector<EdgeIndex> dummy;
        bfs.forwardBfs(g.superSource(), g.superSink(), g, forbidden, dummy);

        const auto& v1 = bfs.v1Mask();

        for (EdgeIndex e : cut)
        {
            VertexIndex u = g.edgeSource(e);
            VertexIndex w = g.edgeTarget(e);
            EXPECT_TRUE(v1.test(u)) << "Edge " << e << " source must be in V1";
            EXPECT_FALSE(v1.test(w)) << "Edge " << e << " target must NOT be in V1";
        }
    }
}

TEST(TestApdfsSolver, C_001_EmptyGraph)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(1);
    auto g = Graph(std::move(builder));
    auto result = findAllCuts(g);

    EXPECT_TRUE(result.empty());
}

TEST(TestApdfsSolver, C_002_Disconnected)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(3);
    builder.addEdge(0, 1);
    builder.addEdge(2, 3);
    auto g = Graph(std::move(builder));
    auto result = findAllCuts(g);

    EXPECT_TRUE(result.empty());
}

TEST(TestApdfsSolver, C_003_ParallelEdges)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(2);
    builder.addEdge(0, 1);
    builder.addEdge(0, 1);
    builder.addEdge(1, 2);
    auto g = Graph(std::move(builder));
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 2U);
    auto it = std::find(result.begin(), result.end(), std::vector<EdgeIndex>{0, 1});
    EXPECT_NE(it, result.end());
}

TEST(TestApdfsSolver, C_004_UnreachableCycle)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(2);
    builder.addEdges({{0, 1}, {1, 2}, {3, 4}, {4, 3}});
    auto g = Graph(std::move(builder));
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 2U);
}

TEST(TestApdfsSolver, C_005_CycleOnPath)
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(3);
    builder.addEdges({{0, 1}, {1, 2}, {2, 3}, {1, 4}, {4, 1}});
    auto g = Graph(std::move(builder));
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 3U);
    for (const auto& cut : result)
    {
        EXPECT_EQ(cut.size(), 1U);
    }
}

TEST(TestApdfsSolver, C_006_IsolatedVertex)
{
    auto g = buildIsolatedVertexGraph();
    auto result = findAllCuts(g);

    ASSERT_EQ(result.size(), 2U);
}

TEST(TestApdfsSolver, C_007_RandomGraphWithCycles)
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
        uint64_t r = rng() % 100;
        if (r < 20 && v >= u)
        {
            std::swap(u, v);
        }
        builder.addEdge(u, v);
    }

    builder.setSuperSource(0);
    builder.setSuperSink(sink);
    auto g = Graph(std::move(builder));
    auto result = findAllCuts(g);

    EXPECT_GT(result.size(), 0U);

    for (const auto& cut : result)
    {
        DynamicBitset forbidden(g.edgeCount());
        for (auto e : cut)
        {
            forbidden.set(e);
        }

        BfsEngine bfs(g.vertexCount(), g.edgeCount());
        std::vector<EdgeIndex> dummy;
        bool reachedSink = bfs.forwardBfs(g.superSource(), g.superSink(), g, forbidden, dummy);
        EXPECT_FALSE(reachedSink) << "Cut does not separate S* and T*";
    }

    for (const auto& cut : result)
    {
        for (size_t skip = 0; skip < cut.size(); ++skip)
        {
            DynamicBitset forbidden(g.edgeCount());
            for (size_t j = 0; j < cut.size(); ++j)
            {
                if (j != skip)
                {
                    forbidden.set(cut[j]);
                }
            }

            BfsEngine bfs(g.vertexCount(), g.edgeCount());
            std::vector<EdgeIndex> dummy;
            bool reachedSink = bfs.forwardBfs(g.superSource(), g.superSink(), g, forbidden, dummy);
            EXPECT_TRUE(reachedSink) << "Cut is not minimal: removing edge " << cut[skip] << " still blocks";
        }
    }

    std::set<std::vector<EdgeIndex>> unique(result.begin(), result.end());
    EXPECT_EQ(unique.size(), result.size()) << "Duplicate cuts found";
}

TEST(TestApdfsSolver, D_001_Determinism)
{
    auto g = buildDiamond();
    auto result1 = findAllCuts(g);
    auto result2 = findAllCuts(g);
    EXPECT_EQ(result1, result2);
}

TEST(TestApdfsSolver, D_002_RepeatedRuns)
{
    auto g = buildDiamond();
    auto first = findAllCuts(g);

    for (int run = 0; run < 10; ++run)
    {
        auto result = findAllCuts(g);
        EXPECT_EQ(result, first) << "Run " << run << " differs";
    }
}

TEST(TestApdfsSolver, D_003_EdgesSortedInResult)
{
    auto g = buildDiamond();
    auto result = findAllCuts(g);

    for (const auto& cut : result)
    {
        for (size_t i = 1; i < cut.size(); ++i)
        {
            EXPECT_LT(cut[i - 1], cut[i]) << "Edges in cut must be sorted";
        }
    }

    for (size_t i = 1; i < result.size(); ++i)
    {
        EXPECT_LT(result[i - 1], result[i]) << "Cuts must be sorted lexicographically";
    }
}

TEST(TestApdfsSolver, D_004_MultiThreadDeterminism_Diamond)
{
    auto g = buildDiamond();
    auto first = findAllCuts(g, 4);

    for (int run = 0; run < 10; ++run)
    {
        auto result = findAllCuts(g, 4);
        EXPECT_EQ(result, first) << "Run " << run << " differs with 4 threads";
    }
}

TEST(TestApdfsSolver, D_005_MultiThreadDeterminism_Random)
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
    auto g = Graph(std::move(builder));

    auto first = findAllCuts(g, 4);
    size_t expectedCount = first.size();

    for (int run = 0; run < 10; ++run)
    {
        auto result = findAllCuts(g, 4);
        EXPECT_EQ(result.size(), expectedCount)
            << "Run " << run << ": expected " << expectedCount << " cuts, got " << result.size();
    }
}

TEST(TestApdfsSolver, F_001_MultiThreadStress)
{
    auto g = buildDiamond();
    auto first = findAllCuts(g, 4);

    for (int run = 0; run < 50; ++run)
    {
        auto result = findAllCuts(g, 4);
        EXPECT_EQ(result, first) << "Stress test run " << run << " failed";
    }
}

TEST(TestApdfsSolver, F_002_DeterminismUnderThreadScaling)
{
    auto g = buildDiamond();
    auto expected = findAllCuts(g, 1);

    for (size_t threads : {2, 4, 8})
    {
        auto result = findAllCuts(g, threads);
        EXPECT_EQ(result, expected) << "Thread count " << threads << " differs from single-threaded";
    }
}

class ChainOfDiamondsTest : public ::testing::TestWithParam<size_t>
{
  protected:
    static Graph buildChainOfDiamonds(size_t k)
    {
        GraphBuilder builder;
        VertexIndex currentSource = 0;
        VertexIndex nextVertex = 1;

        for (size_t d = 0; d < k; ++d)
        {
            VertexIndex in = currentSource;
            VertexIndex a = nextVertex++;
            VertexIndex b = nextVertex++;
            VertexIndex out = nextVertex++;

            builder.addEdge(in, a);
            builder.addEdge(in, b);
            builder.addEdge(a, out);
            builder.addEdge(b, out);

            currentSource = out;
        }

        builder.setSuperSource(0);
        builder.setSuperSink(currentSource);
        return Graph(std::move(builder));
    }
};

TEST_P(ChainOfDiamondsTest, AllCutsValid)
{
    size_t k = GetParam();
    auto g = buildChainOfDiamonds(k);
    auto result = findAllCuts(g);

    for (const auto& cut : result)
    {
        DynamicBitset forbidden(g.edgeCount());
        for (auto e : cut)
        {
            forbidden.set(e);
        }

        BfsEngine bfs(g.vertexCount(), g.edgeCount());
        std::vector<EdgeIndex> dummy;
        bool reachedSink = bfs.forwardBfs(g.superSource(), g.superSink(), g, forbidden, dummy);
        EXPECT_FALSE(reachedSink) << "Invalid cut in chain of " << k << " diamonds";
    }

    for (const auto& cut : result)
    {
        EXPECT_EQ(cut.size(), 2U) << "Cut cardinality must be 2 in chain of diamonds";
    }

    size_t expectedCuts = 4 * k;
    EXPECT_EQ(result.size(), expectedCuts)
        << "K=" << k << ": expected " << expectedCuts << " cuts, got " << result.size();
}

INSTANTIATE_TEST_SUITE_P(ChainOfDiamonds, ChainOfDiamondsTest, ::testing::Values(1, 2, 3, 4, 5));
