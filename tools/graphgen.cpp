#include "core/Xoroshiro128.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace apdfs;

static void generateChain(size_t k, const std::string& outputFile)
{
    std::ofstream f(outputFile);

    size_t vertexCount = 3 * k + 1;
    size_t edgeCount = 4 * k + 2;

    f << "# Chain of " << k << " diamonds\n";
    f << "# Vertices: " << vertexCount << " (plus S* and T*)\n";
    f << "# Edges: " << edgeCount << "\n";
    f << vertexCount << " " << edgeCount << "\n";

    for (size_t i = 0; i < k; ++i)
    {
        size_t in = 3 * i;
        size_t a = in + 1;
        size_t b = in + 2;
        size_t out = in + 3;

        f << in << " " << a << "\n";
        f << in << " " << b << "\n";
        f << a << " " << out << "\n";
        f << b << " " << out << "\n";
    }

    f.close();
    std::cout << "Generated " << outputFile << ": " << vertexCount << " vertices, " << edgeCount << " edges\n";
}

static void generateRandom(size_t numVertices, size_t numEdges, double sideBranchRate, double cycleRate,
                           const std::string& outputFile)
{
    Xoroshiro128 rng(42);

    std::vector<std::pair<size_t, size_t>> edges;
    edges.reserve(numEdges);

    for (size_t i = 0; i < numVertices - 1; ++i)
    {
        edges.emplace_back(i, i + 1);
    }

    size_t mainPathEdges = numVertices - 1;
    size_t remaining = numEdges - mainPathEdges;

    for (size_t i = 0; i < remaining; ++i)
    {
        size_t u = rng() % numVertices;
        size_t v = rng() % numVertices;

        if (u == v)
        {
            --i;
            continue;
        }

        uint64_t r = rng() % 100;

        if (r < cycleRate * 100)
        {
            if (v >= u)
            {
                std::swap(u, v);
            }
        }
        else if (r < (cycleRate + sideBranchRate) * 100)
        {
            if (v <= u)
            {
                std::swap(u, v);
            }
            if (v == u + 1)
            {
                v = u + 2;
            }
        }

        edges.emplace_back(u, v);
    }

    std::mt19937 mt(static_cast<unsigned>(rng()));
    std::shuffle(edges.begin() + static_cast<long>(mainPathEdges), edges.end(), mt);

    std::ofstream f(outputFile);
    f << "# Random graph: " << numVertices << " vertices, " << numEdges << " edges\n";
    f << "# Side branch rate: " << (sideBranchRate * 100) << "%\n";
    f << "# Cycle rate: " << (cycleRate * 100) << "%\n";
    f << numVertices << " " << numEdges << "\n";

    for (const auto& [u, v] : edges)
    {
        f << u << " " << v << "\n";
    }

    f.close();
    std::cout << "Generated " << outputFile << ": " << numVertices << " vertices, " << numEdges << " edges\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage:\n";
        std::cerr << "  " << argv[0] << " chain <K> [output_file]\n";
        std::cerr << "  " << argv[0] << " random <vertices> <edges> <side_rate> <cycle_rate> [output_file]\n";
        std::cerr << "Example:\n";
        std::cerr << "  " << argv[0] << " random 20 60 0.2 0.2 graph.txt\n";
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "chain")
    {
        if (argc < 3)
        {
            std::cerr << "Missing K parameter\n";
            return 1;
        }
        size_t k = std::strtoull(argv[2], nullptr, 10);
        std::string outputFile = "chain_" + std::to_string(k) + ".txt";
        if (argc >= 4)
        {
            outputFile = argv[3];
        }
        generateChain(k, outputFile);
    }
    else if (mode == "random")
    {
        if (argc < 6)
        {
            std::cerr << "Missing parameters: <vertices> <edges> <side_rate> <cycle_rate>\n";
            return 1;
        }

        size_t nv = std::strtoull(argv[2], nullptr, 10);
        size_t ne = std::strtoull(argv[3], nullptr, 10);
        double sideRate = std::strtod(argv[4], nullptr);
        double cycleRate = std::strtod(argv[5], nullptr);

        std::string outputFile = "random_" + std::to_string(nv) + "_" + std::to_string(ne) + ".txt";
        if (argc >= 7)
        {
            outputFile = argv[6];
        }

        generateRandom(nv, ne, sideRate, cycleRate, outputFile);
    }
    else
    {
        std::cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}
