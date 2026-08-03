#include "graph/GraphLoader.hpp"

#include "graph/GraphBuilder.hpp"

#include <fstream>
#include <sstream>

namespace apdfs
{

std::optional<Graph> GraphLoader::loadFromFile(const Config& config)
{
    return loadEdgeList(config);
}

std::optional<Graph> GraphLoader::loadEdgeList(const Config& config)
{
    std::ifstream file(config.filePath);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    GraphBuilder builder;

    std::string line;
    size_t vertexCount = 0;
    size_t edgeCount = 0;
    bool headerParsed = false;

    while (std::getline(file, line))
    {
        auto commentPos = line.find('#');
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
        }
        if (line.find_first_not_of(" \t") == std::string::npos)
        {
            continue;
        }

        std::istringstream iss(line);

        if (!headerParsed)
        {
            if (!(iss >> vertexCount >> edgeCount))
            {
                return std::nullopt;
            }
            headerParsed = true;
            continue;
        }

        VertexIndex u;
        VertexIndex v;
        if (!(iss >> u >> v))
        {
            return std::nullopt;
        }

        builder.addEdge(u, v);
    }

    auto superSource = static_cast<VertexIndex>(vertexCount);
    auto superSink = static_cast<VertexIndex>(vertexCount + 1);

    builder.addEdge(superSource, config.superSource);
    builder.addEdge(config.superSink, superSink);
    builder.setSuperSource(superSource);
    builder.setSuperSink(superSink);

    return Graph(std::move(builder));
}

} // namespace apdfs
