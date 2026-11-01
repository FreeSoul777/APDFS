#include "app/Options.hpp"
#include "core/Stats.hpp"
#include "cut/ApdfsSolver.hpp"
#include "graph/GraphLoader.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>

#ifndef APDFS_VERSION
#define APDFS_VERSION "unknown"
#endif

using namespace apdfs;

int main(int argc, char* argv[])
{
    const OptionsResult parsed = parseOptions(argc, argv);
    if (!parsed.ok())
    {
        std::cerr << "error: " << parsed.error << "\n\n" << usageText();
        return 2;
    }

    const Options& options = parsed.options;
    if (options.help)
    {
        std::cout << usageText();
        return 0;
    }
    if (options.version)
    {
        std::cout << "apdfs " << APDFS_VERSION << "\n";
        return 0;
    }

    GraphLoader::Config loaderConfig;
    loaderConfig.filePath = options.graphFile;
    loaderConfig.superSource = options.source;
    loaderConfig.superSink = options.sink;

    auto loaded = GraphLoader::loadFromFile(loaderConfig);
    if (!loaded.has_value())
    {
        std::cerr << "error: failed to load graph: " << options.graphFile << "\n";
        return 1;
    }

    SolverConfig solverConfig;
    solverConfig.outputDir = options.outputDir;
    solverConfig.threadCount = options.threads;
    solverConfig.showProgress = options.showProgress;
    solverConfig.maxCuts = options.maxCuts;
    solverConfig.maxCutSize = options.maxCutSize;
    solverConfig.maxExamined = options.maxExamined;
    solverConfig.collectInMemory = false;

    const Graph& graph = *loaded;

    std::cout << "Graph: " << graph.vertexCount() << " vertices, " << graph.edgeCount() << " edges\n";
    std::cout << "Source: " << options.source << ", Sink: " << options.sink << "\n";
    std::cout << "Threads: " << options.threads << "\n";
    std::cout << "Output: " << solverConfig.outputDir << "\n";
    if (options.maxCuts > 0)
    {
        std::cout << "Max cuts: " << options.maxCuts << "\n";
    }
    if (options.maxCutSize > 0)
    {
        std::cout << "Max cut size: " << options.maxCutSize << " edges\n";
    }
    if (options.maxExamined > 0)
    {
        std::cout << "Max examined: " << options.maxExamined << "\n";
    }
    std::cout << "\n";

    const size_t memBefore = getPeakMemoryBytes();
    const auto startTime = std::chrono::steady_clock::now();

    ApdfsSolver solver(graph, solverConfig);
    solver.run();

    if (solver.hadWriteError())
    {
        std::cerr << "error: writing the output failed; the dataset is incomplete\n";
        return 1;
    }

    const auto endTime = std::chrono::steady_clock::now();
    const size_t memAfter = getPeakMemoryBytes();
    const auto& stats = solver.stats();

    std::cout << "\nResults:\n";
    std::cout << "  Total cuts: " << stats.cutsFound.load() << "\n";
    if (options.maxCutSize > 0)
    {
        std::cout << "  Filtered by size: " << stats.cutsFiltered.load() << "\n";
    }
    std::cout << "  Frames: " << stats.framesProcessed.load() << "\n";
    std::cout << "  Duplicates: " << stats.duplicatesSkipped.load() << " (" << std::fixed << std::setprecision(1)
              << stats.duplicateRate() << "%)\n";
    std::cout << "  Duration: " << std::chrono::duration<double>(endTime - startTime).count() << " s\n";
    std::cout << "  Speed: " << std::fixed << std::setprecision(0) << stats.cutsPerSecond() << " cuts/s\n";
    std::cout << "  Memory: " << static_cast<double>(memAfter - memBefore) / 1024.0 / 1024.0 << " MB\n";
    std::cout << "  Dir: " << solverConfig.outputDir << "\n";

    return 0;
}
