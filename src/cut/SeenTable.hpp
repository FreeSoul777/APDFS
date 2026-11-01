#pragma once

#include "core/SpinLock.hpp"
#include "core/Types.hpp"

#include <atomic>
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

    bool tryMarkProcessing(Hash key) noexcept;
    bool contains(Hash key) noexcept;

    void markDone(Hash) noexcept
    {
    }

    size_t size() const noexcept;

    size_t numSegments() const noexcept
    {
        return mNumSegments;
    }

  private:
    static constexpr double MAX_LOAD_FACTOR = 0.5;
    static constexpr size_t INITIAL_CAPACITY = 64;

    struct alignas(64) SegmentLock
    {
        SpinLock lock;
    };

    struct SegmentData
    {
        std::vector<Hash> mKeys;
        std::vector<uint64_t> mOccupiedBits;
        size_t mOccupied = 0;
        size_t mCapacityMask = INITIAL_CAPACITY - 1;

        SegmentData()
        {
            allocate(INITIAL_CAPACITY);
        }

        void allocate(size_t capacity)
        {
            mKeys.assign(capacity, Hash{});
            mOccupiedBits.assign((capacity + 63) / 64, 0);
            mCapacityMask = capacity - 1;
            mOccupied = 0;
        }

        bool isOccupied(size_t index) const noexcept
        {
            return ((mOccupiedBits[index >> 6] >> (index & 63)) & 1U) != 0U;
        }

        void setOccupied(size_t index) noexcept
        {
            mOccupiedBits[index >> 6] |= (uint64_t{1} << (index & 63));
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

        void rehash();
        void forceInsert(Hash key) noexcept;
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

    std::atomic<size_t> mEntryCount{0};
};

} // namespace apdfs
