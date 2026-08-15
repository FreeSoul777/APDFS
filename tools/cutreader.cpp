#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

static constexpr uint32_t MAGIC = 0x4544484D;
static constexpr uint32_t VERSION = 1;

struct EdgeInfo
{
    uint32_t source;
    uint32_t target;
};

static std::unordered_map<uint32_t, EdgeInfo> loadMapFile(const std::filesystem::path& mapPath)
{
    std::unordered_map<uint32_t, EdgeInfo> result;
    std::ifstream f(mapPath);
    if (!f.is_open())
    {
        return result;
    }

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        std::istringstream iss(line);
        uint32_t idx;
        uint32_t src;
        uint32_t tgt;
        if (iss >> idx >> src >> tgt)
        {
            result[idx] = {src, tgt};
        }
    }
    return result;
}

static void printCut(uint64_t cutIndex, const std::vector<uint32_t>& edges,
                     const std::unordered_map<uint32_t, EdgeInfo>& edgeMap, bool human)
{
    std::cout << "Cut " << cutIndex << ": {";
    for (size_t i = 0; i < edges.size(); ++i)
    {
        if (i > 0)
        {
            std::cout << ", ";
        }
        if (human && (edgeMap.contains(edges[i]) != 0u))
        {
            const auto& info = edgeMap.at(edges[i]);
            std::cout << info.source << "->" << info.target;
        }
        else
        {
            std::cout << edges[i];
        }
    }
    std::cout << "} [" << edges.size() << " edges]\n";
}

static uint64_t countCutsInFile(const std::filesystem::path& binPath)
{
    std::FILE* f = std::fopen(binPath.c_str(), "rb");
    if (f == nullptr)
    {
        return 0;
    }

    uint32_t header[4];
    if (std::fread(header, sizeof(uint32_t), 4, f) != 4 || header[0] != MAGIC || header[1] != VERSION)
    {
        std::fprintf(stderr, "Warning: %s has incompatible format\n", binPath.c_str());
        std::fclose(f);
        return 0;
    }

    uint64_t count = 0;
    uint32_t cutSize;
    while (std::fread(&cutSize, sizeof(uint32_t), 1, f) == 1)
    {
        count++;
        std::fseek(f, cutSize * sizeof(uint32_t), SEEK_CUR);
    }
    std::fclose(f);
    return count;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <output_dir> [--page N] [--human]\n";
        std::cerr << "  Reads all .bin files in output_dir.\n";
        std::cerr << "  --page N: show N cuts at a time, press Enter for next page (default: all)\n";
        std::cerr << "  --human: show edges as source->target (requires edges.map in output_dir)\n";
        std::cerr << "Example:\n";
        std::cerr << "  " << argv[0] << " outdir/out --page 100 --human\n";
        return 1;
    }

    std::filesystem::path outputDir(argv[1]);
    uint64_t pageSize = UINT64_MAX;
    bool human = false;

    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--human")
        {
            human = true;
        }
        else if (arg == "--page" && i + 1 < argc)
        {
            pageSize = std::strtoull(argv[++i], nullptr, 10);
        }
    }

    std::vector<std::filesystem::path> binFiles;
    for (const auto& entry : std::filesystem::directory_iterator(outputDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".bin")
        {
            binFiles.push_back(entry.path());
        }
    }
    std::sort(binFiles.begin(), binFiles.end());

    if (binFiles.empty())
    {
        std::cerr << "No .bin files found in " << outputDir << "\n";
        return 1;
    }

    std::unordered_map<uint32_t, EdgeInfo> edgeMap;
    if (human)
    {
        std::filesystem::path mapPath = outputDir / "edges.map";
        edgeMap = loadMapFile(mapPath);
        if (edgeMap.empty())
        {
            std::cerr << "Warning: no edges.map found, showing indices only\n";
            human = false;
        }
    }

    uint64_t totalCuts = 0;
    for (const auto& path : binFiles)
    {
        totalCuts += countCutsInFile(path);
    }

    std::cerr << "Files: " << binFiles.size() << ", Total cuts: " << totalCuts << "\n";
    if (pageSize < totalCuts)
    {
        std::cerr << "Showing " << pageSize << " cuts per page. Press Enter for next page, 'q' to quit.\n\n";
    }

    uint64_t globalIndex = 0;
    uint64_t pageStart = 0;
    bool quit = false;

    for (const auto& binPath : binFiles)
    {
        if (quit)
        {
            break;
        }

        std::FILE* f = std::fopen(binPath.c_str(), "rb");
        if (f == nullptr)
        {
            continue;
        }

        uint32_t header[4];
        size_t headerRead = std::fread(header, sizeof(uint32_t), 4, f);
        if (headerRead != 4)
        {
            std::fclose(f);
            continue;
        }

        uint32_t cutSize;
        std::vector<uint32_t> edges;

        while (std::fread(&cutSize, sizeof(uint32_t), 1, f) == 1 && !quit)
        {
            edges.resize(cutSize);
            size_t edgesRead = std::fread(edges.data(), sizeof(uint32_t), cutSize, f);
            if (edgesRead != cutSize)
            {
                break;
            }

            printCut(globalIndex, edges, edgeMap, human);
            globalIndex++;

            if (pageSize < UINT64_MAX && (globalIndex - pageStart) >= pageSize)
            {
                std::cerr << "\n--- Page " << (pageStart / pageSize + 1) << " (cuts " << pageStart << "-"
                          << (globalIndex - 1) << " of " << totalCuts << ") ---\n";
                std::cerr << "Press Enter for next page, 'q' to quit... " << std::flush;

                std::string input;
                std::getline(std::cin, input);
                if (input == "q" || input == "Q")
                {
                    quit = true;
                    break;
                }
                pageStart = globalIndex;
                std::cerr << "\n";
            }
        }

        std::fclose(f);
    }

    if (!quit)
    {
        std::cerr << "\n--- End (cuts " << pageStart << "-" << (globalIndex - 1) << " of " << totalCuts << ") ---\n";
    }

    return 0;
}
