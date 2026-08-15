#include "cut/ApdfsSolver.hpp"

#include "core/Shutdown.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

namespace apdfs
{

ApdfsSolver::ApdfsSolver(const Graph& graph, SolverConfig config)
    : mGraph(graph), mConfig(std::move(config)), mSeen(SeenTable::computeOptimalSegments(mConfig.threadCount))
{
}

Frame ApdfsSolver::buildInitialFrame()
{
    std::vector<EdgeIndex> pInit;
    Hash hInit{0, 0};

    for (EdgeIndex e : mGraph.outgoingEdges(mGraph.superSource()))
    {
        pInit.push_back(e);
        hInit ^= mGraph.edgeHash(e);
    }

    std::sort(pInit.begin(), pInit.end());
    return Frame{hInit, std::move(pInit), 0};
}

void ApdfsSolver::workerLoop(ThreadContext& ctx)
{
    while (!isShutdownRequested())
    {
        for (size_t batch = 0; batch < 100; ++batch)
        {
            Frame frame;
            bool hasFrame = false;

            {
                std::lock_guard<SpinLock> lock(ctx.roadmapLock);
                if (!ctx.roadmap.empty())
                {
                    frame = std::move(ctx.roadmap.back());
                    ctx.roadmap.pop_back();
                    hasFrame = true;
                }
            }

            if (!hasFrame)
            {
                break;
            }

            if (!mSeen.tryMarkProcessing(frame.cutHash))
            {
                mStats.duplicatesSkipped.fetch_add(1, std::memory_order_relaxed);
                continue;
            }

            std::vector<EdgeIndex> pClean;
            auto children = ctx.processor.processFrame(frame, pClean);

            mStats.framesProcessed.fetch_add(1, std::memory_order_relaxed);

            if (!pClean.empty())
            {
                mStats.cutsFound.fetch_add(1, std::memory_order_relaxed);
                ctx.writer->writeCut(pClean);

                if (mConfig.collectInMemory)
                {
                    std::lock_guard<std::mutex> lock(mAllCutsMutex);
                    mAllCuts.push_back(std::move(pClean));
                }

                if (mConfig.maxCuts > 0 && mStats.cutsFound.load(std::memory_order_relaxed) >= mConfig.maxCuts)
                {
                    gShutdownRequested.store(true, std::memory_order_release);
                }
            }

            std::vector<Frame> spilledToPush;
            {
                std::lock_guard<SpinLock> lock(ctx.roadmapLock);

                for (auto it = children.rbegin(); it != children.rend(); ++it)
                {
                    if (!mSeen.contains(it->cutHash))
                    {
                        ctx.roadmap.push_back(std::move(*it));
                    }
                    else
                    {
                        mStats.duplicatesSkipped.fetch_add(1, std::memory_order_relaxed);
                    }
                }

                if (ctx.roadmap.size() > mConfig.maxLocalFrames)
                {
                    size_t half = ctx.roadmap.size() / 2;
                    spilledToPush.assign(std::make_move_iterator(ctx.roadmap.begin()),
                                         std::make_move_iterator(ctx.roadmap.begin() + static_cast<long>(half)));
                    ctx.roadmap.erase(ctx.roadmap.begin(), ctx.roadmap.begin() + static_cast<long>(half));
                }
            }

            if (!spilledToPush.empty())
            {
                pushToGlobalHeap(spilledToPush);
            }

            mSeen.markDone(frame.cutHash);
            updatePeakSeen(mSeen.size());
        }

        size_t roadmapSize;
        {
            std::lock_guard<SpinLock> lock(ctx.roadmapLock);
            roadmapSize = ctx.roadmap.size();
        }
        updatePeakRoadmap(roadmapSize);

        if (roadmapSize == 0)
        {
            std::vector<Frame> stolen;

            if (trySteal(ctx, stolen))
            {
                std::lock_guard<SpinLock> lock(ctx.roadmapLock);
                ctx.roadmap.insert(ctx.roadmap.end(), std::make_move_iterator(stolen.begin()),
                                   std::make_move_iterator(stolen.end()));
                continue;
            }

            if (popFromGlobalHeap(stolen, 100))
            {
                std::lock_guard<SpinLock> lock(ctx.roadmapLock);
                ctx.roadmap.insert(ctx.roadmap.end(), std::make_move_iterator(stolen.begin()),
                                   std::make_move_iterator(stolen.end()));
                continue;
            }

            ctx.idle.store(true, std::memory_order_release);

            bool allIdle = true;
            for (auto& c : mContexts)
            {
                if (!c->idle.load(std::memory_order_acquire))
                {
                    allIdle = false;
                    break;
                }
            }

            if (allIdle && mGlobalHeapSize.load() == 0)
            {
                gShutdownRequested.store(true, std::memory_order_release);
                mReporterCv.notify_all();
                break;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(100));
            ctx.idle.store(false, std::memory_order_release);
        }
    }

    ctx.writer->flush();
}

bool ApdfsSolver::trySteal(ThreadContext& thief, std::vector<Frame>& outFrames)
{
    outFrames.clear();

    static thread_local std::mt19937 rng(std::random_device{}());
    size_t victimCount = mContexts.size() - 1;
    if (victimCount == 0)
    {
        return false;
    }

    std::uniform_int_distribution<size_t> dist(0, mContexts.size() - 1);

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        size_t victimIdx = dist(rng);
        if (mContexts[victimIdx].get() == &thief || mContexts[victimIdx]->roadmap.size() < 2)
        {
            continue;
        }

        auto& victim = *mContexts[victimIdx];

        std::lock_guard<SpinLock> lock(victim.roadmapLock);

        if (victim.roadmap.size() < 2)
        {
            continue;
        }

        size_t half = victim.roadmap.size() / 2;
        outFrames.insert(outFrames.end(), std::make_move_iterator(victim.roadmap.begin()),
                         std::make_move_iterator(victim.roadmap.begin() + static_cast<long>(half)));
        victim.roadmap.erase(victim.roadmap.begin(), victim.roadmap.begin() + static_cast<long>(half));

        mStats.framesStolen.fetch_add(outFrames.size(), std::memory_order_relaxed);
        return true;
    }

    return false;
}

void ApdfsSolver::pushToGlobalHeap(std::vector<Frame>& frames)
{
    if (frames.empty())
    {
        return;
    }

    size_t count = frames.size();
    for (auto& f : frames)
    {
        mGlobalHeap.enqueue(std::move(f));
    }
    mGlobalHeapSize.fetch_add(count, std::memory_order_release);
    mStats.globalHeapPushes.fetch_add(count, std::memory_order_relaxed);
    frames.clear();
}

bool ApdfsSolver::popFromGlobalHeap(std::vector<Frame>& outFrames, size_t maxCount)
{
    outFrames.clear();
    outFrames.reserve(maxCount);

    Frame f;
    size_t count = 0;
    while (count < maxCount && mGlobalHeap.try_dequeue(f))
    {
        outFrames.push_back(std::move(f));
        count++;
    }

    if (count > 0)
    {
        mGlobalHeapSize.fetch_sub(count, std::memory_order_release);
        return true;
    }
    return false;
}

void ApdfsSolver::run()
{
    gShutdownRequested.store(false, std::memory_order_release);
    installSignalHandlers();
    mStats.reset();

    writeEdgeMap();

    size_t numThreads = std::max(size_t(1), mConfig.threadCount);
    mContexts.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i)
    {
        mContexts.push_back(std::make_unique<ThreadContext>(i, mGraph, mSeen, mConfig.outputDir));
    }

    std::thread reporter;
    if (mConfig.showProgress)
    {
        reporter = std::thread(&ApdfsSolver::statsReporterThread, this);
    }

    {
        DynamicBitset noForbidden(mGraph.edgeCount());
        std::vector<EdgeIndex> dummy;
        bool sinkReachable =
            mContexts[0]->bfs.forwardBfs(mGraph.superSource(), mGraph.superSink(), mGraph, noForbidden, dummy);
        if (!sinkReachable)
        {
            gShutdownRequested.store(true);
            mReporterCv.notify_all();
            if (reporter.joinable())
            {
                reporter.join();
            }
            return;
        }
    }

    Frame initFrame = buildInitialFrame();
    if (!initFrame.cutEdges.empty())
    {
        mContexts[0]->roadmap.push_back(std::move(initFrame));
    }

    for (size_t i = 0; i < numThreads; ++i)
    {
        mThreads.emplace_back(&ApdfsSolver::workerLoop, this, std::ref(*mContexts[i]));
    }

    for (auto& t : mThreads)
    {
        t.join();
    }

    mReporterCv.notify_all();
    if (reporter.joinable())
    {
        reporter.join();
    }

    if (mConfig.showProgress)
    {
        std::cerr << "\n";
        printFinalStats();
    }

    if (mConfig.collectInMemory)
    {
        for (auto& cut : mAllCuts)
        {
            std::sort(cut.begin(), cut.end());
        }
        std::sort(mAllCuts.begin(), mAllCuts.end());
    }
}

void ApdfsSolver::statsReporterThread()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mReporterMutex);
        bool shutdown = mReporterCv.wait_for(lock, std::chrono::milliseconds(mConfig.progressIntervalMs),
                                             [] { return isShutdownRequested(); });

        if (shutdown)
        {
            break;
        }

        printProgressLine();
    }
}

void ApdfsSolver::printProgressLine() const
{
    uint64_t cuts = mStats.cutsFound.load(std::memory_order_relaxed);
    uint64_t dups = mStats.duplicatesSkipped.load(std::memory_order_relaxed);
    uint64_t stolen = mStats.framesStolen.load(std::memory_order_relaxed);
    double elapsed = mStats.elapsedSeconds();
    double rate = (elapsed > 0.001) ? static_cast<double>(cuts) / elapsed : 0.0;
    double dupRate = mStats.duplicateRate();
    size_t peakMem = mStats.peakMemoryBytes();

    std::string eta;
    if (mConfig.maxCuts > 0 && cuts > 0 && rate > 0)
    {
        double remaining = static_cast<double>(mConfig.maxCuts - cuts) / rate;
        if (remaining > 1.0)
        {
            int mins = static_cast<int>(remaining) / 60;
            int secs = static_cast<int>(remaining) % 60;
            if (mins > 0)
            {
                eta = "ETA: " + std::to_string(mins) + "m" + std::to_string(secs) + "s";
            }
            else
            {
                eta = "ETA: " + std::to_string(secs) + "s";
            }
        }
    }

    uint64_t totalBytes = 0;
    for (const auto& c : mContexts)
    {
        totalBytes += c->writer->totalBytesWritten();
    }

    std::cerr << "\r" << "Cuts: " << cuts << " | Dups: " << dups << " (" << dupRate << "%)" << " | Stolen: " << stolen
              << " | Heap: " << mGlobalHeapSize.load() << " | Time: " << static_cast<uint64_t>(elapsed) << "s"
              << " | Speed: " << static_cast<uint64_t>(rate) << " cuts/s" << " | Mem: " << (peakMem / 1024 / 1024)
              << " MB" << " | File: " << (totalBytes / 1024 / 1024) << " MB";

    if (!eta.empty())
    {
        std::cerr << " | " << eta;
    }
    std::cerr << "                    \r" << std::flush;
}

void ApdfsSolver::updatePeakRoadmap(size_t currentSize) noexcept
{
    size_t peak = mStats.peakRoadmapSize.load(std::memory_order_relaxed);
    while (currentSize > peak &&
           !mStats.peakRoadmapSize.compare_exchange_weak(peak, currentSize, std::memory_order_relaxed))
    {
    }
}

void ApdfsSolver::updatePeakSeen(size_t currentSize) noexcept
{
    size_t peak = mStats.peakSeenSize.load(std::memory_order_relaxed);
    while (currentSize > peak &&
           !mStats.peakSeenSize.compare_exchange_weak(peak, currentSize, std::memory_order_relaxed))
    {
    }
}

void ApdfsSolver::printFinalStats() const
{
    uint64_t totalBytes = 0;
    uint32_t totalFiles = 0;
    for (const auto& c : mContexts)
    {
        totalBytes += c->writer->totalBytesWritten();
        totalFiles += c->writer->filesRotated() + 1;
    }

    std::cerr << "=== Final Stats ===\n";
    std::cerr << "Frames processed: " << mStats.framesProcessed.load() << "\n";
    std::cerr << "Cuts found:       " << mStats.cutsFound.load() << "\n";
    std::cerr << "Duplicates:       " << mStats.duplicatesSkipped.load() << " (" << std::fixed << std::setprecision(1)
              << mStats.duplicateRate() << "%)\n";
    std::cerr << "Stolen frames:    " << mStats.framesStolen.load() << "\n";
    std::cerr << "GlobalHeap ops:   " << mStats.globalHeapPushes.load() << "\n";
    std::cerr << "Time:             " << std::fixed << std::setprecision(3) << mStats.elapsedSeconds() << " s\n";
    std::cerr << "Speed:            " << std::fixed << std::setprecision(0) << mStats.cutsPerSecond() << " cuts/s\n";
    std::cerr << "Peak memory:      " << std::fixed << std::setprecision(2)
              << (static_cast<double>(mStats.peakMemoryBytes()) / 1024.0 / 1024.0) << " MB\n";
    std::cerr << "Peak roadmap:     " << mStats.peakRoadmapSize.load() << " frames\n";
    std::cerr << "Peak Seen:        " << mStats.peakSeenSize.load() << " entries\n";
    std::cerr << "Files written:    " << totalFiles << "\n";
    std::cerr << "Total written:    " << (static_cast<double>(totalBytes) / 1024.0 / 1024.0) << " MB\n";
}

void ApdfsSolver::writeEdgeMap() const
{
    std::filesystem::path mapPath = mConfig.outputDir / "edges.map";
    std::ofstream f(mapPath);
    if (!f.is_open())
    {
        return;
    }

    f << "# Edge index -> source target\n";
    f << "# Total edges: " << mGraph.edgeCount() << "\n";
    for (EdgeIndex e = 0; e < static_cast<EdgeIndex>(mGraph.edgeCount()); ++e)
    {
        f << e << " " << mGraph.edgeSource(e) << " " << mGraph.edgeTarget(e) << "\n";
    }
}

} // namespace apdfs
