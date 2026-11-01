#pragma once

#include "core/SpinLock.hpp"
#include "cut/CutProcessor.hpp"

#include <algorithm>
#include <atomic>
#include <vector>

namespace apdfs
{

class SpillBuffer
{
  public:
    SpillBuffer() = default;
    SpillBuffer(const SpillBuffer&) = delete;
    SpillBuffer& operator=(const SpillBuffer&) = delete;
    SpillBuffer(SpillBuffer&&) = delete;
    SpillBuffer& operator=(SpillBuffer&&) = delete;

    void reserve(size_t capacity)
    {
        std::lock_guard<SpinLock> guard(mLock);
        mFrames.reserve(capacity);
    }

    void push(std::vector<Frame>& frames)
    {
        if (frames.empty())
        {
            return;
        }

        std::lock_guard<SpinLock> guard(mLock);
        for (auto& frame : frames)
        {
            mFrames.push_back(std::move(frame));
        }
        mSize.store(mFrames.size(), std::memory_order_release);
        frames.clear();
    }

    bool pop(std::vector<Frame>& outFrames, size_t maxCount)
    {
        outFrames.clear();

        std::lock_guard<SpinLock> guard(mLock);
        if (mFrames.empty())
        {
            return false;
        }

        const size_t count = std::min(maxCount, mFrames.size());
        outFrames.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            outFrames.push_back(std::move(mFrames.back()));
            mFrames.pop_back();
        }
        mSize.store(mFrames.size(), std::memory_order_release);
        return true;
    }

    size_t size() const noexcept
    {
        return mSize.load(std::memory_order_acquire);
    }

  private:
    mutable SpinLock mLock;
    std::vector<Frame> mFrames;
    std::atomic<size_t> mSize{0};
};

} // namespace apdfs
