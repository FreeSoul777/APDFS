#pragma once

#include "core/Types.hpp"
#include "cut/BfsEngine.hpp"
#include "cut/SeenTable.hpp"
#include "graph/Graph.hpp"

#include <vector>

namespace apdfs
{

struct Frame
{
    Hash cutHash;
    std::vector<EdgeIndex> cutEdges;
    size_t iterator;

    Frame() = default;

    Frame(Hash h, std::vector<EdgeIndex>&& e, size_t i) noexcept : cutHash(h), cutEdges(std::move(e)), iterator(i)
    {
    }
};

class CutProcessor
{
  public:
    CutProcessor(BfsEngine& bfs, const Graph& graph, SeenTable& seen);

    std::vector<Frame> processFrame(const Frame& frame, std::vector<EdgeIndex>& pCleanOut);

    void collectCandidates(const DynamicBitset& v1, std::vector<EdgeIndex>& outCandidates);

  private:
    BfsEngine& mBfs;
    const Graph& mGraph;
    SeenTable& mSeen;

    std::vector<EdgeIndex> mCandidatesBuffer;
    std::vector<EdgeIndex> mCleanCutBuffer;

    bool reconstructCut(const Frame& frame, std::vector<EdgeIndex>& outPActual);

    std::vector<Frame> branchFrame(const Frame& frame, const std::vector<EdgeIndex>& pClean,
                                   const DynamicBitset& v1Saved);

    bool cleanToMinimalCut(const std::vector<EdgeIndex>& candidates, const DynamicBitset& v1New,
                           std::vector<EdgeIndex>& outCleanCut);
};

} // namespace apdfs
