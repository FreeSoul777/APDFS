
#include "cut/ApdfsSolver.hpp"
#include "graph/GraphBuilder.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

using namespace apdfs;

namespace
{

struct RunResult
{
    size_t cuts = 0;
    double seconds = 0.0;
};

RunResult runSolver(const Graph& graph, size_t threads)
{
    SolverConfig config;
    config.outputDir = "outdir/apdfs_perf";
    config.threadCount = threads;
    config.collectInMemory = true;
    config.showProgress = false;

    std::filesystem::create_directories(config.outputDir);

    const auto start = std::chrono::steady_clock::now();
    ApdfsSolver solver(graph, config);
    solver.run();
    const auto finish = std::chrono::steady_clock::now();

    RunResult result;
    result.cuts = solver.getCuts().size();
    result.seconds = std::chrono::duration<double>(finish - start).count();
    return result;
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

Graph buildChainOfDiamonds(size_t k)
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

} // namespace

TEST(TestPerformance, Random20x60_SingleThread_CompletesUnderBudget)
{
    auto graph = buildRandom20x60();
    const auto result = runSolver(graph, 1);

    EXPECT_GT(result.cuts, 100U) << "expected a few thousand minimal cuts";
    EXPECT_LT(result.seconds, 5.0) << "took " << result.seconds << " s for " << result.cuts << " cuts";
}

TEST(TestPerformance, Random20x60_FourThreads_CompletesUnderBudget)
{
    auto graph = buildRandom20x60();
    const auto single = runSolver(graph, 1);
    const auto parallel = runSolver(graph, 4);

    EXPECT_EQ(parallel.cuts, single.cuts) << "thread count must not change the result set size";
    EXPECT_LT(parallel.seconds, 5.0) << "took " << parallel.seconds << " s for " << parallel.cuts << " cuts";
}

TEST(TestPerformance, ChainOfDiamonds_K10_CompletesUnderBudget)
{
    auto graph = buildChainOfDiamonds(10);
    const auto result = runSolver(graph, 1);

    EXPECT_EQ(result.cuts, 40U);
    EXPECT_LT(result.seconds, 2.0) << "took " << result.seconds << " s";
}
