#include "core/Stats.hpp"
#include "cut/ApdfsSolver.hpp"
#include "graph/GraphLoader.hpp"

#include <sys/resource.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

using namespace apdfs;

namespace
{

bool parseUint(const char* str, unsigned long& out)
{
    if ((str == nullptr) || *str == '\0')
    {
        return false;
    }
    char* end = nullptr;
    out = std::strtoul(str, &end, 10);
    return *end == '\0';
}

bool parseVertexId(const char* str, VertexIndex& out)
{
    unsigned long val = 0;
    if (!parseUint(str, val))
    {
        return false;
    }
    out = static_cast<VertexIndex>(val);
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <graph_file> <source> <sink> [threads] [output_dir] [--progress] [--max-cuts=N]\n";
        std::cerr << "Example: " << argv[0] << " graph.txt 0 7 4 output --progress --max-cuts=50000\n";
        return 1;
    }

    std::string path = argv[1];
    VertexIndex source = 0;
    VertexIndex sink = 0;
    size_t threads = std::thread::hardware_concurrency();
    std::filesystem::path outputDir = std::filesystem::current_path() / "output";
    bool showProgress = false;
    uint64_t maxCuts = 0;

    if (!parseVertexId(argv[2], source) || !parseVertexId(argv[3], sink))
    {
        std::cerr << "Invalid source or sink argument\n";
        return 1;
    }

    int posIdx = 4;
    if (argc > posIdx)
    {
        unsigned long val = 0;
        if (parseUint(argv[posIdx], val) && val > 0)
        {
            threads = static_cast<size_t>(val);
            posIdx++;
        }
    }
    if (argc > posIdx)
    {
        std::string arg = argv[posIdx];
        if (!arg.starts_with("--"))
        {
            outputDir = arg;
            posIdx++;
        }
    }

    for (int i = posIdx; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--progress")
        {
            showProgress = true;
        }
        else if (arg.starts_with("--max-cuts="))
        {
            unsigned long val = 0;
            if (parseUint(arg.substr(11).c_str(), val) && val > 0)
            {
                maxCuts = static_cast<uint64_t>(val);
            }
        }
    }

    GraphLoader::Config loaderCfg;
    loaderCfg.filePath = path;
    loaderCfg.superSource = source;
    loaderCfg.superSink = sink;

    auto graphResult = GraphLoader::loadFromFile(loaderCfg);
    if (!graphResult.has_value())
    {
        std::cerr << "Failed to load graph from: " << path << "\n";
        return 1;
    }

    const Graph& g = *graphResult;

    SolverConfig solverCfg;
    solverCfg.outputDir = outputDir;
    solverCfg.threadCount = threads;
    solverCfg.showProgress = showProgress;
    solverCfg.maxCuts = maxCuts;
    solverCfg.collectInMemory = false;

    std::filesystem::create_directories(solverCfg.outputDir);

    std::cout << "=== Edgehammer ===\n";
    std::cout << "Graph: " << g.vertexCount() << " vertices, " << g.edgeCount() << " edges\n";
    std::cout << "Source: " << source << ", Sink: " << sink << "\n";
    std::cout << "Threads: " << threads << "\n";
    std::cout << "Output: " << solverCfg.outputDir << "\n";
    if (maxCuts > 0)
    {
        std::cout << "Max cuts: " << maxCuts << "\n";
    }
    std::cout << "\n";

    size_t memBefore = getPeakMemoryBytes();
    auto startTime = std::chrono::steady_clock::now();

    ApdfsSolver solver(g, solverCfg);
    solver.run();

    auto endTime = std::chrono::steady_clock::now();
    size_t memAfter = getPeakMemoryBytes();

    const auto& stats = solver.stats();

    std::cout << "\nResults:\n";
    std::cout << "  Total cuts: " << stats.cutsFound.load() << "\n";
    std::cout << "  Frames processed: " << stats.framesProcessed.load() << "\n";
    std::cout << "  Duplicates: " << stats.duplicatesSkipped.load() << " (" << std::fixed << std::setprecision(1)
              << stats.duplicateRate() << "%)\n";
    std::cout << "  Duration: " << std::chrono::duration<double>(endTime - startTime).count() << " s\n";
    std::cout << "  Speed: " << std::fixed << std::setprecision(0) << stats.cutsPerSecond() << " cuts/s\n";
    std::cout << "  Memory: " << static_cast<double>(memAfter - memBefore) / 1024.0 / 1024.0 << " MB\n";
    std::cout << "  Dir: " << solverCfg.outputDir << "\n";

    return 0;
}
