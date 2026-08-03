#include "cut/BfsEngine.hpp"

#include <algorithm>
#include <cstring>

namespace apdfs
{

BfsEngine::BfsEngine(size_t vertexCount, size_t edgeCount)
    : mV1(vertexCount), mVT(vertexCount), mCutMask(edgeCount)
{
    mQueue.reserve(vertexCount);
}

bool BfsEngine::forwardBfs(VertexIndex source, VertexIndex sink, const Graph& graph,
                           const DynamicBitset& forbiddenEdges, std::vector<EdgeIndex>& outCutEdges)
{
    mV1.resetAll();
    outCutEdges.clear();

    size_t head = 0;
    size_t tail = 0;
    mQueue.clear();
    mQueue.push_back(source);
    mV1.set(source);
    tail++;

    while (head < tail)
    {
        VertexIndex u = mQueue[head++];

        for (EdgeIndex e : graph.outgoingEdges(u))
        {
            if (forbiddenEdges.test(e))
            {
                VertexIndex w = graph.edgeTarget(e);
                if (!mV1.test(w))
                {
                    outCutEdges.push_back(e);
                }
                continue;
            }

            VertexIndex w = graph.edgeTarget(e);

            if (!mV1.test(w))
            {
                mV1.set(w);
                mQueue.push_back(w);
                tail++;
            }
        }
    }

    return mV1.test(sink);
}

bool BfsEngine::incrementalForwardBfs(VertexIndex fromVertex, VertexIndex sink, const Graph& graph,
                                      const DynamicBitset& forbiddenEdges)
{
    size_t head = 0;
    size_t tail = 0;
    mQueue.clear();

    if (!mV1.test(fromVertex))
    {
        mQueue.push_back(fromVertex);
        mV1.set(fromVertex);
        tail++;
    }

    while (head < tail)
    {
        VertexIndex u = mQueue[head++];

        for (EdgeIndex e : graph.outgoingEdges(u))
        {
            if (forbiddenEdges.test(e))
            {
                continue;
            }

            VertexIndex w = graph.edgeTarget(e);

            if (!mV1.test(w))
            {
                mV1.set(w);
                mQueue.push_back(w);
                tail++;

                if (w == sink)
                {
                    return true;
                }
            }
        }
    }

    return mV1.test(sink);
}

void BfsEngine::backwardBfs(VertexIndex sink, const Graph& graph, const DynamicBitset& forbiddenEdges)
{
    mVT.resetAll();

    size_t head = 0;
    size_t tail = 0;
    mQueue.clear();

    mQueue.push_back(sink);
    mVT.set(sink);
    tail++;

    while (head < tail)
    {
        VertexIndex u = mQueue[head++];

        for (EdgeIndex e : graph.incomingEdges(u))
        {
            if (forbiddenEdges.test(e))
            {
                continue;
            }

            VertexIndex w = graph.edgeSource(e);

            if (!mVT.test(w))
            {
                mVT.set(w);
                mQueue.push_back(w);
                tail++;
            }
        }
    }
}

void BfsEngine::filterByVT(const Graph& graph, const DynamicBitset& v1New, const DynamicBitset& vt,
                           const std::vector<EdgeIndex>& candidates, std::vector<EdgeIndex>& outCleanCut)
{
    outCleanCut.clear();

    for (EdgeIndex e : candidates)
    {
        VertexIndex u = graph.edgeSource(e);
        VertexIndex w = graph.edgeTarget(e);

        if (v1New.test(u) && vt.test(w))
        {
            outCleanCut.push_back(e);
        }
    }
}

} // namespace apdfs
