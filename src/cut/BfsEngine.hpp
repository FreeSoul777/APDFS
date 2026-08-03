#pragma once

#include "core/Bitset.hpp"
#include "core/Types.hpp"
#include "graph/Graph.hpp"

#include <cstdint>
#include <vector>

namespace apdfs
{

class BfsEngine
{
  public:
    BfsEngine(size_t vertexCount, size_t edgeCount);

    bool forwardBfs(VertexIndex source, VertexIndex sink, const Graph& graph, const DynamicBitset& forbiddenEdges,
                    std::vector<EdgeIndex>& outCutEdges);

    bool incrementalForwardBfs(VertexIndex fromVertex, VertexIndex sink, const Graph& graph,
                               const DynamicBitset& forbiddenEdges);

    void backwardBfs(VertexIndex sink, const Graph& graph, const DynamicBitset& forbiddenEdges);

    static void filterByVT(const Graph& graph, const DynamicBitset& v1New, const DynamicBitset& vt,
                           const std::vector<EdgeIndex>& candidates, std::vector<EdgeIndex>& outCleanCut);

    DynamicBitset& v1Mask() noexcept
    {
        return mV1;
    }

    DynamicBitset& vtMask() noexcept
    {
        return mVT;
    }

    DynamicBitset& cutMask() noexcept
    {
        return mCutMask;
    }

    const DynamicBitset& v1Mask() const noexcept
    {
        return mV1;
    }

    const DynamicBitset& vtMask() const noexcept
    {
        return mVT;
    }

    const DynamicBitset& cutMask() const noexcept
    {
        return mCutMask;
    }

  private:
    DynamicBitset mV1;
    DynamicBitset mVT;
    DynamicBitset mCutMask;
    std::vector<VertexIndex> mQueue;
};

} // namespace apdfs
