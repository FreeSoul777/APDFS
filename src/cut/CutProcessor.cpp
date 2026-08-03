#include "cut/CutProcessor.hpp"

#include <algorithm>

namespace apdfs
{

CutProcessor::CutProcessor(BfsEngine& bfs, const Graph& graph, SeenTable& seen) : mBfs(bfs), mGraph(graph), mSeen(seen)
{
}

std::vector<Frame> CutProcessor::processFrame(const Frame& frame, std::vector<EdgeIndex>& pCleanOut)
{
    if (!reconstructCut(frame, pCleanOut))
    {
        return {};
    }

    DynamicBitset v1Saved = mBfs.v1Mask();

    std::vector<Frame> newFrames = branchFrame(frame, pCleanOut, v1Saved);

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

    for (size_t i = frame.iterator; i < pClean.size(); ++i)
    {
        EdgeIndex ei = pClean[i];
        VertexIndex v = mGraph.edgeTarget(ei);

        if (v == mGraph.superSink())
        {
            continue;
        }

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
        collectCandidates(mBfs.v1Mask(), mCandidatesBuffer);

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
            continue;
        }
        newFrames.emplace_back();
        newFrames.back().cutHash = hNew;
        newFrames.back().cutEdges.swap(mCleanCutBuffer);
        newFrames.back().iterator = 0;
    }

    return newFrames;
}

void CutProcessor::collectCandidates(const DynamicBitset& v1, std::vector<EdgeIndex>& outCandidates)
{
    outCandidates.clear();
    v1.forEachSet([&](size_t u) {
        auto vu = static_cast<VertexIndex>(u);
        for (EdgeIndex e : mGraph.outgoingEdges(vu))
        {
            VertexIndex w = mGraph.edgeTarget(e);
            if (!v1.test(w))
            {
                outCandidates.push_back(e);
            }
        }
    });
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
    mBfs.filterByVT(mGraph, v1New, mBfs.vtMask(), candidates, outCleanCut);

    return !outCleanCut.empty();
}

} // namespace apdfs
