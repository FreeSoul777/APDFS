#pragma once

#include "concurrentqueue.h"

#include "core/Stats.hpp"
#include "core/Types.hpp"
#include "cut/BfsEngine.hpp"
#include "cut/CutProcessor.hpp"
#include "cut/CutWriter.hpp"
#include "cut/SeenTable.hpp"
#include "graph/Graph.hpp"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace apdfs
{

struct SolverConfig
{
    size_t threadCount = 1;
    size_t progressIntervalMs = 200;
    size_t maxLocalFrames = 10000;
    bool showProgress = false;
    bool collectInMemory = false;
    std::filesystem::path outputDir = "output";
    uint64_t maxCuts = 0;
};

class ApdfsSolver
{
  public:
    ApdfsSolver(const Graph& graph, SolverConfig config);
    void run();

    const SolverStats& stats() const noexcept
    {
        return mStats;
    }

    const std::vector<std::vector<EdgeIndex>>& getCuts() const noexcept
    {
        return mAllCuts;
    }

  private:
    const Graph& mGraph;
    const SolverConfig mConfig;
    SeenTable mSeen;
    SolverStats mStats;

    struct alignas(64) ThreadContext
    {
        size_t threadId;
        BfsEngine bfs;
        CutProcessor processor;
        std::unique_ptr<CutWriter> writer;
        std::vector<Frame> roadmap;
        SpinLock roadmapLock;
        std::atomic<bool> idle{false};

        ThreadContext(size_t id, const Graph& graph, SeenTable& seen, const std::filesystem::path& outputDir)
            : threadId(id),
              bfs(graph.vertexCount(), graph.edgeCount()),
              processor(bfs, graph, seen),
              writer(std::make_unique<CutWriter>(outputDir, static_cast<uint32_t>(id)))
        {
            roadmap.reserve(256);
        }
    };

    std::vector<std::unique_ptr<ThreadContext>> mContexts;
    std::vector<std::thread> mThreads;
    std::vector<std::vector<EdgeIndex>> mAllCuts;
    std::mutex mAllCutsMutex;

    moodycamel::ConcurrentQueue<Frame> mGlobalHeap;
    std::atomic<uint64_t> mGlobalHeapSize{0};

    std::mutex mReporterMutex;
    std::condition_variable mReporterCv;

    Frame buildInitialFrame();
    void workerLoop(ThreadContext& ctx);
    bool trySteal(ThreadContext& thief, std::vector<Frame>& outFrames);
    void pushToGlobalHeap(std::vector<Frame>& frames);
    bool popFromGlobalHeap(std::vector<Frame>& outFrames, size_t maxCount);

    void statsReporterThread();
    void printProgressLine() const;
    void updatePeakRoadmap(size_t currentSize) noexcept;
    void updatePeakSeen(size_t currentSize) noexcept;
    void printFinalStats() const;
    void writeEdgeMap() const;
};

} // namespace apdfs
