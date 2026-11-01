#pragma once

#include "core/Types.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace apdfs
{

class GraphBuilder;

class Graph
{
  public:
    struct Edge
    {
        VertexIndex source;
        VertexIndex target;
    };

    explicit Graph(GraphBuilder&& builder);

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph(Graph&&) noexcept = default;
    Graph& operator=(Graph&&) noexcept = default;

    std::span<const EdgeIndex> outgoingEdges(VertexIndex v) const noexcept;
    std::span<const EdgeIndex> incomingEdges(VertexIndex v) const noexcept;

    const Edge& edge(EdgeIndex idx) const noexcept
    {
        return mEdges[idx];
    }

    Hash edgeHash(EdgeIndex idx) const noexcept
    {
        return mEdgeHashes[idx];
    }

    VertexIndex edgeSource(EdgeIndex idx) const noexcept
    {
        return mEdges[idx].source;
    }

    VertexIndex edgeTarget(EdgeIndex idx) const noexcept
    {
        return mEdges[idx].target;
    }

    size_t vertexCount() const noexcept
    {
        return mVertexCount;
    }

    size_t edgeCount() const noexcept
    {
        return mEdges.size();
    }

    VertexIndex superSource() const noexcept
    {
        return mSuperSource;
    }

    VertexIndex superSink() const noexcept
    {
        return mSuperSink;
    }

    size_t outDegree(VertexIndex v) const noexcept;
    size_t inDegree(VertexIndex v) const noexcept;

  private:
    size_t mVertexCount;
    VertexIndex mSuperSource;
    VertexIndex mSuperSink;

    std::vector<Edge> mEdges;
    std::vector<Hash> mEdgeHashes;

    std::vector<EdgeIndex> mOutAdj;
    std::vector<size_t> mOutOffset;

    std::vector<EdgeIndex> mIncAdj;
    std::vector<size_t> mIncOffset;

    void buildCsr();
};

} // namespace apdfs
