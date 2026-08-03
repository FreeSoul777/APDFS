#include "graph/Graph.hpp"

#include "graph/GraphBuilder.hpp"

#include <algorithm>

namespace apdfs
{

Graph::Graph(GraphBuilder&& builder)
{
    builder.assignSuperVertices();

    mVertexCount = builder.mMaxVertex;
    mSuperSource = builder.mSuperSource;
    mSuperSink = builder.mSuperSink;

    size_t edgeCount = builder.mEdges.size();
    mEdges.reserve(edgeCount);
    mEdgeHashes.reserve(edgeCount);

    for (auto& e : builder.mEdges)
    {
        mEdges.push_back({e.source, e.target});
        mEdgeHashes.push_back(e.hash);
    }

    buildCsr();
}

void Graph::buildCsr()
{
    std::vector<size_t> outDeg(mVertexCount, 0);
    std::vector<size_t> inDeg(mVertexCount, 0);

    for (EdgeIndex e = 0; e < static_cast<EdgeIndex>(mEdges.size()); ++e)
    {
        outDeg[mEdges[e].source]++;
        inDeg[mEdges[e].target]++;
    }

    mOutOffset.resize(mVertexCount + 1);
    mOutOffset[0] = 0;
    for (size_t v = 0; v < mVertexCount; ++v)
    {
        mOutOffset[v + 1] = mOutOffset[v] + outDeg[v];
    }
    mOutAdj.resize(mOutOffset[mVertexCount]);

    mIncOffset.resize(mVertexCount + 1);
    mIncOffset[0] = 0;
    for (size_t v = 0; v < mVertexCount; ++v)
    {
        mIncOffset[v + 1] = mIncOffset[v] + inDeg[v];
    }
    mIncAdj.resize(mIncOffset[mVertexCount]);

    std::vector<size_t> outCursor = mOutOffset;
    std::vector<size_t> incCursor = mIncOffset;

    for (EdgeIndex e = 0; e < static_cast<EdgeIndex>(mEdges.size()); ++e)
    {
        VertexIndex u = mEdges[e].source;
        VertexIndex v = mEdges[e].target;

        mOutAdj[outCursor[u]++] = e;
        mIncAdj[incCursor[v]++] = e;
    }
}

std::span<const EdgeIndex> Graph::outgoingEdges(VertexIndex v) const noexcept
{
    if (v >= mVertexCount)
    {
        return {};
    }
    size_t start = mOutOffset[v];
    size_t end = mOutOffset[v + 1];
    return {mOutAdj.data() + start, end - start};
}

std::span<const EdgeIndex> Graph::incomingEdges(VertexIndex v) const noexcept
{
    if (v >= mVertexCount)
    {
        return {};
    }
    size_t start = mIncOffset[v];
    size_t end = mIncOffset[v + 1];
    return {mIncAdj.data() + start, end - start};
}

size_t Graph::outDegree(VertexIndex v) const noexcept
{
    if (v >= mVertexCount)
    {
        return 0;
    }
    return mOutOffset[v + 1] - mOutOffset[v];
}

size_t Graph::inDegree(VertexIndex v) const noexcept
{
    if (v >= mVertexCount)
    {
        return 0;
    }
    return mIncOffset[v + 1] - mIncOffset[v];
}

} // namespace apdfs
