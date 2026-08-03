#pragma once

#include <sys/resource.h>

#include <atomic>
#include <chrono>
#include <cstdint>

namespace apdfs
{

inline size_t getPeakMemoryBytes()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return static_cast<size_t>(usage.ru_maxrss) * 1024;
}

struct SolverStats
{
    std::atomic<uint64_t> framesProcessed{0};
    std::atomic<uint64_t> cutsFound{0};
    std::atomic<uint64_t> duplicatesSkipped{0};
    std::atomic<uint64_t> framesStolen{0};
    std::atomic<uint64_t> globalHeapPushes{0};
    std::atomic<uint64_t> bytesWritten{0};
    std::atomic<uint32_t> filesRotated{0};

    std::atomic<size_t> peakRoadmapSize{0};
    std::atomic<size_t> peakSeenSize{0};

    size_t memBefore{0};
    std::chrono::steady_clock::time_point startTime;

    SolverStats() = default;
    SolverStats(const SolverStats&) = delete;
    SolverStats& operator=(const SolverStats&) = delete;
    SolverStats(SolverStats&&) = delete;
    SolverStats& operator=(SolverStats&&) = delete;

    void reset() noexcept
    {
        framesProcessed = 0;
        cutsFound = 0;
        duplicatesSkipped = 0;
        framesStolen = 0;
        globalHeapPushes = 0;
        bytesWritten = 0;
        filesRotated = 0;
        peakRoadmapSize = 0;
        peakSeenSize = 0;
        memBefore = getPeakMemoryBytes();
        startTime = std::chrono::steady_clock::now();
    }

    size_t peakMemoryBytes() const noexcept
    {
        size_t current = getPeakMemoryBytes();
        return (current > memBefore) ? (current - memBefore) : 0;
    }

    double elapsedSeconds() const noexcept
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - startTime).count();
    }

    double cutsPerSecond() const noexcept
    {
        double sec = elapsedSeconds();
        if (sec <= 0.0)
        {
            return 0.0;
        }
        return static_cast<double>(cutsFound.load(std::memory_order_relaxed)) / sec;
    }

    double duplicateRate() const noexcept
    {
        uint64_t total =
            framesProcessed.load(std::memory_order_relaxed) + duplicatesSkipped.load(std::memory_order_relaxed);
        if (total == 0)
        {
            return 0.0;
        }
        return 100.0 * static_cast<double>(duplicatesSkipped.load(std::memory_order_relaxed)) /
               static_cast<double>(total);
    }
};

} // namespace apdfs
