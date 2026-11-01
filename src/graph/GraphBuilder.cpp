#include "graph/GraphBuilder.hpp"

#include <algorithm>

namespace apdfs
{

GraphBuilder::GraphBuilder()
    : mMaxVertex(0),
      mSuperSource(0),
      mSuperSink(0),
      mSuperSourceSet(false),
      mSuperSinkSet(false),
      mRng(Xoroshiro128::DEFAULT_SEED)
{
}

void GraphBuilder::addEdge(VertexIndex source, VertexIndex target)
{
    mMaxVertex = std::max(mMaxVertex, static_cast<VertexIndex>(std::max(source, target) + 1));

    uint64_t hi = mRng();
    uint64_t lo = mRng();
    mEdges.push_back({source, target, Hash{lo, hi}});
}

void GraphBuilder::addEdges(const std::vector<std::pair<VertexIndex, VertexIndex>>& edges)
{
    for (const auto& [u, v] : edges)
    {
        addEdge(u, v);
    }
}

void GraphBuilder::setSuperSource(VertexIndex source)
{
    mSuperSource = source;
    mSuperSourceSet = true;
    mMaxVertex = std::max(mMaxVertex, static_cast<VertexIndex>(source + 1));
}

void GraphBuilder::setSuperSink(VertexIndex sink)
{
    mSuperSink = sink;
    mSuperSinkSet = true;
    mMaxVertex = std::max(mMaxVertex, static_cast<VertexIndex>(sink + 1));
}

void GraphBuilder::assignSuperVertices()
{
    if (!mSuperSourceSet)
    {
        mSuperSource = mMaxVertex;
        mMaxVertex++;
    }
    if (!mSuperSinkSet)
    {
        mSuperSink = mMaxVertex;
        mMaxVertex++;
    }
}

} // namespace apdfs
