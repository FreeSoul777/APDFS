#include "cut/CutProcessor.hpp"

#include <algorithm>

namespace apdfs
{

CutProcessor::CutProcessor(BfsEngine& bfs, const Graph& graph, SeenTable& seen)
    : mBfs(bfs), mGraph(graph), mSeen(seen), mV1Saved(graph.vertexCount()), mHeadSeen(graph.vertexCount())
{
}

std::vector<Frame> CutProcessor::processFrame(const Frame& frame, std::vector<EdgeIndex>& pCleanOut)
{
    if (!reconstructCut(frame, pCleanOut))
    {
        return {};
    }

    if (mV1Saved.size() != mBfs.v1Mask().size())
    {
        mV1Saved = DynamicBitset(mBfs.v1Mask().size());
    }
    mV1Saved.copyFrom(mBfs.v1Mask());

    std::vector<Frame> newFrames = branchFrame(frame, pCleanOut, mV1Saved);

    return newFrames;
}

bool CutProcessor::reconstructCut(const Frame& frame, std::vector<EdgeIndex>& outPActual)
{
    DynamicBitset& forbidden = mBfs.cutMask();
    forbidden.resetAll();
    for (EdgeIndex e : frame.cutEdges)
    {
        forbidden.set(e);
    }

    std::vector<EdgeIndex> pActual;
    bool reachedSink = mBfs.forwardBfs(mGraph.superSource(), mGraph.superSink(), mGraph, forbidden, outPActual);

    return !reachedSink;
}

std::vector<Frame> CutProcessor::branchFrame(const Frame& frame, const std::vector<EdgeIndex>& pClean,
                                             const DynamicBitset& v1Saved)
{
    std::vector<Frame> newFrames;

    const size_t start = std::min(frame.iterator, pClean.size());
    newFrames.reserve(pClean.size() - start);

    mHeadSeen.resetAll();

    for (size_t i = start; i < pClean.size(); ++i)
    {
        EdgeIndex ei = pClean[i];
        VertexIndex v = mGraph.edgeTarget(ei);

        if (v == mGraph.superSink())
        {
            continue;
        }

        if (mHeadSeen.test(v))
        {
            continue;
        }
        mHeadSeen.set(v);

        auto outgoing = mGraph.outgoingEdges(v);
        if (outgoing.empty())
        {
            continue;
        }

        mBfs.v1Mask().copyFrom(v1Saved);

        DynamicBitset& qMask = mBfs.cutMask();
        qMask.resetAll();
        for (EdgeIndex e : pClean)
        {
            if (e != ei)
            {
                qMask.set(e);
            }
        }
        for (EdgeIndex e : outgoing)
        {
            qMask.set(e);
        }

        bool reachedSink = mBfs.incrementalForwardBfs(v, mGraph.superSink(), mGraph, qMask);
        if (reachedSink)
        {
            continue;
        }

        mCandidatesBuffer.clear();
        for (EdgeIndex boundaryEdge : pClean)
        {
            if (!mBfs.v1Mask().test(mGraph.edgeTarget(boundaryEdge)))
            {
                mCandidatesBuffer.push_back(boundaryEdge);
            }
        }
        for (EdgeIndex outEdge : outgoing)
        {
            if (!mBfs.v1Mask().test(mGraph.edgeTarget(outEdge)))
            {
                mCandidatesBuffer.push_back(outEdge);
            }
        }

        mCleanCutBuffer.clear();
        if (!cleanToMinimalCut(mCandidatesBuffer, mBfs.v1Mask(), mCleanCutBuffer))
        {
            continue;
        }

        Hash hNew{0, 0};
        for (EdgeIndex e : mCleanCutBuffer)
        {
            hNew ^= mGraph.edgeHash(e);
        }

        if (mSeen.contains(hNew))
        {
            ++mFilteredDuplicates;
            continue;
        }
        newFrames.emplace_back();
        newFrames.back().cutHash = hNew;
        newFrames.back().cutEdges.swap(mCleanCutBuffer);
        newFrames.back().iterator = 0;
    }

    return newFrames;
}

bool CutProcessor::cleanToMinimalCut(const std::vector<EdgeIndex>& candidates, const DynamicBitset& v1New,
                                     std::vector<EdgeIndex>& outCleanCut)
{
    if (candidates.empty())
    {
        outCleanCut.clear();
        return false;
    }

    DynamicBitset& candMask = mBfs.cutMask();
    candMask.resetAll();
    for (EdgeIndex e : candidates)
    {
        candMask.set(e);
    }

    mBfs.backwardBfs(mGraph.superSink(), mGraph, candMask);

    outCleanCut.clear();
    outCleanCut.reserve(candidates.size());
    mBfs.filterByVT(mGraph, v1New, mBfs.vtMask(), candidates, outCleanCut);

    return !outCleanCut.empty();
}

} // namespace apdfs
