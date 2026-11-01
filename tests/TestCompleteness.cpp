#include "TestOracles.hpp"

#include "cut/ApdfsSolver.hpp"
#include "graph/GraphBuilder.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <set>
#include <string>
#include <vector>

using namespace apdfs;

namespace
{

Graph buildRandomGraph(uint64_t seed, size_t vertices, size_t extraEdges)
{
    Xoroshiro128 rng(seed);
    GraphBuilder builder;

    for (size_t v = 0; v + 1 < vertices; ++v)
    {
        builder.addEdge(static_cast<VertexIndex>(v), static_cast<VertexIndex>(v + 1));
    }

    for (size_t i = 0; i < extraEdges; ++i)
    {
        auto u = static_cast<VertexIndex>(rng() % vertices);
        auto v = static_cast<VertexIndex>(rng() % vertices);
        builder.addEdge(u, v);
    }

    builder.setSuperSource(0);
    builder.setSuperSink(static_cast<VertexIndex>(vertices - 1));
    return Graph(std::move(builder));
}

std::set<std::vector<EdgeIndex>> bruteForceMinimalCuts(const Graph& graph)
{
    const size_t edgeCount = graph.edgeCount();
    const VertexIndex source = graph.superSource();
    const VertexIndex sink = graph.superSink();
    std::set<std::vector<EdgeIndex>> result;

    for (size_t mask = 0; mask < (size_t{1} << edgeCount); ++mask)
    {
        std::vector<EdgeIndex> candidate;
        for (size_t e = 0; e < edgeCount; ++e)
        {
            if (((mask >> e) & 1U) != 0U)
            {
                candidate.push_back(static_cast<EdgeIndex>(e));
            }
        }

        if (!test::isValidCut(graph, source, sink, candidate))
        {
            continue;
        }

        bool minimal = true;
        for (EdgeIndex removed : candidate)
        {
            std::vector<EdgeIndex> smaller;
            for (EdgeIndex e : candidate)
            {
                if (e != removed)
                {
                    smaller.push_back(e);
                }
            }
            if (test::isValidCut(graph, source, sink, smaller))
            {
                minimal = false;
                break;
            }
        }

        if (minimal)
        {
            result.insert(std::move(candidate));
        }
    }

    return result;
}

std::vector<std::vector<EdgeIndex>> solve(const Graph& graph, size_t threads)
{
    SolverConfig config;
    config.outputDir = "outdir/apdfs_completeness";
    config.threadCount = threads;
    config.collectInMemory = true;
    config.showProgress = false;

    std::filesystem::create_directories(config.outputDir);

    ApdfsSolver solver(graph, config);
    solver.run();
    return solver.getCuts();
}

std::string formatCut(const std::vector<EdgeIndex>& cut)
{
    std::string text = "{";
    for (size_t i = 0; i < cut.size(); ++i)
    {
        if (i != 0)
        {
            text += ",";
        }
        text += std::to_string(cut[i]);
    }
    text += "}";
    return text;
}

void expectSameCutSets(const std::set<std::vector<EdgeIndex>>& expected, const std::set<std::vector<EdgeIndex>>& actual,
                       uint64_t seed)
{
    std::vector<std::vector<EdgeIndex>> missing;
    std::vector<std::vector<EdgeIndex>> extra;
    std::set_difference(expected.begin(), expected.end(), actual.begin(), actual.end(), std::back_inserter(missing));
    std::set_difference(actual.begin(), actual.end(), expected.begin(), expected.end(), std::back_inserter(extra));

    EXPECT_TRUE(missing.empty()) << "seed=" << seed << ": solver missed " << missing.size()
                                 << " minimal cut(s), first: " << formatCut(missing.front());
    EXPECT_TRUE(extra.empty()) << "seed=" << seed << ": solver reported " << extra.size()
                               << " non-minimal cut(s), first: " << formatCut(extra.front());
    EXPECT_EQ(actual.size(), expected.size()) << "seed=" << seed << ": cut count differs";
}

} // namespace

TEST(TestCompleteness, Exhaustive_SmallGraphs_SingleThread)
{
    size_t maxCutCount = 0;
    for (uint64_t seed = 1; seed <= 20; ++seed)
    {
        auto graph = buildRandomGraph(seed, 5, 3);
        auto expected = bruteForceMinimalCuts(graph);

        ASSERT_FALSE(expected.empty()) << "seed=" << seed << ": oracle found no cuts";
        maxCutCount = std::max(maxCutCount, expected.size());

        auto cuts = solve(graph, 1);
        std::set<std::vector<EdgeIndex>> actual(cuts.begin(), cuts.end());

        expectSameCutSets(expected, actual, seed);
    }

    EXPECT_GT(maxCutCount, 2U);
}

TEST(TestCompleteness, Exhaustive_DenserGraphs_SingleThread)
{
    for (uint64_t seed = 100; seed <= 108; ++seed)
    {
        auto graph = buildRandomGraph(seed, 6, 4);
        auto expected = bruteForceMinimalCuts(graph);
        auto cuts = solve(graph, 1);
        std::set<std::vector<EdgeIndex>> actual(cuts.begin(), cuts.end());

        expectSameCutSets(expected, actual, seed);
    }
}

TEST(TestCompleteness, Exhaustive_MultiThread_MatchesOracle)
{
    for (uint64_t seed = 200; seed <= 204; ++seed)
    {
        auto graph = buildRandomGraph(seed, 6, 3);
        auto expected = bruteForceMinimalCuts(graph);
        auto cuts = solve(graph, 4);
        std::set<std::vector<EdgeIndex>> actual(cuts.begin(), cuts.end());

        expectSameCutSets(expected, actual, seed);
    }
}

TEST(TestCompleteness, InvariantsHoldOnRandomGraphs)
{
    for (uint64_t seed = 300; seed <= 312; ++seed)
    {
        auto graph = buildRandomGraph(seed, 7, 5);
        auto cuts = solve(graph, 1);

        ASSERT_FALSE(cuts.empty()) << "seed=" << seed;

        std::set<std::vector<EdgeIndex>> unique;
        for (const auto& cut : cuts)
        {
            EXPECT_FALSE(cut.empty()) << "seed=" << seed << ": empty cut";
            EXPECT_TRUE(unique.insert(cut).second) << "seed=" << seed << ": duplicate cut " << formatCut(cut);

            EXPECT_TRUE(std::is_sorted(cut.begin(), cut.end())) << "seed=" << seed << ": cut is not sorted";

            EXPECT_TRUE(test::isValidCut(graph, graph.superSource(), graph.superSink(), cut))
                << "seed=" << seed << ": " << formatCut(cut) << " does not separate S* and T*";

            for (EdgeIndex removed : cut)
            {
                std::vector<EdgeIndex> smaller;
                for (EdgeIndex e : cut)
                {
                    if (e != removed)
                    {
                        smaller.push_back(e);
                    }
                }
                EXPECT_FALSE(test::isValidCut(graph, graph.superSource(), graph.superSink(), smaller))
                    << "seed=" << seed << ": " << formatCut(cut) << " is not minimal, edge " << removed
                    << " has no free path";
            }
        }
    }
}

TEST(TestCompleteness, ChainOfDiamonds_CountMatchesClosedForm)
{
    for (size_t k = 1; k <= 6; ++k)
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
        auto graph = Graph(std::move(builder));

        auto cuts = solve(graph, 1);
        EXPECT_EQ(cuts.size(), 4 * k) << "K=" << k;
    }
}
