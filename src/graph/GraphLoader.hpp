#pragma once

#include "core/Types.hpp"
#include "graph/Graph.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace apdfs
{

class GraphLoader
{
  public:
    struct Config
    {
        std::filesystem::path filePath;
        VertexIndex superSource;
        VertexIndex superSink;
    };

    static std::optional<Graph> loadFromFile(const Config& config);

  private:
    static std::optional<Graph> loadEdgeList(const Config& config);
};

} // namespace apdfs
