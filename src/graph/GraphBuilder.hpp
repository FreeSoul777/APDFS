#pragma once

#include "core/Types.hpp"
#include "core/Xoroshiro128.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace apdfs
{

class Graph;

class GraphBuilder
{
    friend class Graph;

  public:
    GraphBuilder();

    void addEdge(VertexIndex source, VertexIndex target);
    void setSuperSource(VertexIndex source);
    void setSuperSink(VertexIndex sink);

    void addEdges(const std::vector<std::pair<VertexIndex, VertexIndex>>& edges);

  private:
    struct Edge
    {
        VertexIndex source;
        VertexIndex target;
        Hash hash;
    };

    VertexIndex mMaxVertex;
    VertexIndex mSuperSource;
    VertexIndex mSuperSink;
    bool mSuperSourceSet;
    bool mSuperSinkSet;
    std::vector<Edge> mEdges;
    Xoroshiro128 mRng;

    void assignSuperVertices();
};

} // namespace apdfs
