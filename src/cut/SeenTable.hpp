#pragma once

#include "core/SpinLock.hpp"
#include "core/Types.hpp"

#include <cstdint>
#include <vector>

namespace apdfs
{

class SeenTable
{
  public:
    static size_t computeOptimalSegments(size_t threadCount) noexcept
    {
        size_t segs = std::max(size_t(1024), threadCount * 256);
        segs = nextPowerOfTwo(segs);
        return std::min(segs, size_t(65536));
    }

    explicit SeenTable(size_t numSegments = 4096) : mNumSegments(numSegments), mSegments(numSegments)
    {
    }

    enum class State : uint8_t
    {
        PROCESSING = 0,
        DONE = 1
    };

    bool tryMarkProcessing(Hash key) noexcept;
    bool contains(Hash key) noexcept;
    void markDone(Hash key) noexcept;
    size_t size() const noexcept;

    size_t numSegments() const noexcept
    {
        return mNumSegments;
    }

  private:
    static constexpr double MAX_LOAD_FACTOR = 0.5;
    static constexpr size_t INITIAL_CAPACITY = 64;

    struct Slot
    {
        Hash key;
        State state;
        uint8_t distance;
        bool occupied;
    };

    struct alignas(64) SegmentLock
    {
        SpinLock lock;
    };

    struct SegmentData
    {
        std::vector<Slot> mSlots;
        size_t mOccupied = 0;
        size_t mCapacityMask = INITIAL_CAPACITY - 1;

        SegmentData()
        {
            mSlots.resize(INITIAL_CAPACITY);
            clearSlots();
        }

        void clearSlots() noexcept
        {
            for (auto& slot : mSlots)
            {
                slot.occupied = false;
                slot.distance = 0;
            }
        }
    };

    struct alignas(64) Segment
    {
        SegmentLock mLock;
        SegmentData mData;

        bool tryInsert(Hash key) noexcept;
        bool contains(Hash key) noexcept;
        void markDone(Hash key) noexcept;

      private:
        size_t hashToIndex(Hash key) const noexcept
        {
            size_t h = key.lo ^ (key.hi >> 32) ^ (key.hi << 16);
            return h & mData.mCapacityMask;
        }

        void rehash() noexcept;
        void forceInsert(Hash key, State state) noexcept;
    };

    size_t segmentIndex(Hash key) const noexcept
    {
        return key.lo & (mNumSegments - 1);
    }

    Segment& segment(Hash key) noexcept
    {
        return mSegments[segmentIndex(key)];
    }

    static size_t nextPowerOfTwo(size_t n) noexcept
    {
        if (n == 0)
        {
            return 1;
        }
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    size_t mNumSegments;
    std::vector<Segment> mSegments;
};

} // namespace apdfs
