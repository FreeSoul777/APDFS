#include "core/Stats.hpp"
#include "cut/ApdfsSolver.hpp"
#include "graph/GraphBuilder.hpp"

#include <benchmark/benchmark.h>

#include <filesystem>

using namespace apdfs;

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

static Graph buildRandomGraph(size_t nv, size_t ne, double cycleRate, uint64_t seed)
{
    GraphBuilder builder;
    auto sink = static_cast<VertexIndex>(nv - 1);
    Xoroshiro128 rng(seed);

    for (VertexIndex i = 0; i < sink; ++i)
    {
        builder.addEdge(i, i + 1);
    }

    size_t remaining = ne - static_cast<size_t>(sink);
    for (size_t i = 0; i < remaining; ++i)
    {
        auto u = static_cast<VertexIndex>(rng() % nv);
        auto v = static_cast<VertexIndex>(rng() % nv);
        if (u == v)
        {
            --i;
            continue;
        }
        if (rng() % 100 < cycleRate * 100 && v >= u)
        {
            std::swap(u, v);
        }
        builder.addEdge(u, v);
    }

    builder.setSuperSource(0);
    builder.setSuperSink(sink);
    return Graph(std::move(builder));
}

static void runChainBenchmark(benchmark::State& state, size_t threadCount)
{
    size_t k = state.range(0);
    auto g = buildChainOfDiamonds(k);

    {
        SolverConfig cfg;
        cfg.outputDir = std::filesystem::temp_directory_path() / "apdfs_bench";
        cfg.threadCount = threadCount;
        ApdfsSolver solver(g, cfg);
        solver.run();
    }

    for (auto _ : state)
    {
        SolverConfig cfg;
        cfg.outputDir = std::filesystem::temp_directory_path() / "apdfs_bench";
        cfg.threadCount = threadCount;
        cfg.collectInMemory = false;

        std::filesystem::remove_all(cfg.outputDir);
        ApdfsSolver solver(g, cfg);
        solver.run();

        state.counters["Cuts"] = 4.0 * k;
    }
}

static void bmChain1T(benchmark::State& state)
{
    runChainBenchmark(state, 1);
}

static void bmChain4T(benchmark::State& state)
{
    runChainBenchmark(state, 4);
}

static void runRandomBenchmark(benchmark::State& state, size_t threadCount)
{
    auto g = buildRandomGraph(20, 60, 0.2, 42);

    {
        SolverConfig cfg;
        cfg.outputDir = std::filesystem::temp_directory_path() / "apdfs_bench";
        cfg.threadCount = threadCount;
        ApdfsSolver(g, cfg).run();
    }

    for (auto _ : state)
    {
        SolverConfig cfg;
        cfg.outputDir = std::filesystem::temp_directory_path() / "apdfs_bench";
        cfg.threadCount = threadCount;
        std::filesystem::remove_all(cfg.outputDir);
        ApdfsSolver solver(g, cfg);
        solver.run();
    }
}

static void bmRandom1T(benchmark::State& state)
{
    runRandomBenchmark(state, 1);
}

static void bmRandom4T(benchmark::State& state)
{
    runRandomBenchmark(state, 4);
}

BENCHMARK(bmChain1T)->Arg(10)->Arg(12)->Arg(15)->Arg(20)->Unit(benchmark::kMillisecond);
BENCHMARK(bmChain4T)->Arg(10)->Arg(12)->Arg(15)->Arg(20)->Unit(benchmark::kMillisecond);
BENCHMARK(bmRandom1T)->Unit(benchmark::kMillisecond);
BENCHMARK(bmRandom4T)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
