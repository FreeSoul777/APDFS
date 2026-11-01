#pragma once

#include <immintrin.h>

#include <atomic>

namespace apdfs
{

class alignas(64) SpinLock
{
  public:
    SpinLock() noexcept : mLock(UNLOCKED)
    {
    }

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    void lock() noexcept
    {
        for (int backoff = 0; backoff < MAX_BACKOFF; ++backoff)
        {
            if (tryLock())
            {
                return;
            }
            for (int i = 0; i < (1 << backoff); ++i)
            {
                _mm_pause();
            }
        }

        while (!tryLock())
        {
            _mm_pause();
        }
    }

    bool tryLock() noexcept
    {
        return mLock.exchange(LOCKED, std::memory_order_acquire) == UNLOCKED;
    }

    void unlock() noexcept
    {
        mLock.store(UNLOCKED, std::memory_order_release);
    }

  private:
    static constexpr uint32_t UNLOCKED = 0;
    static constexpr uint32_t LOCKED = 1;
    static constexpr int MAX_BACKOFF = 10;

    std::atomic<uint32_t> mLock;
};

static_assert(sizeof(SpinLock) == 64, "SpinLock must be exactly one cache line");
static_assert(alignof(SpinLock) == 64, "SpinLock must be 64-byte aligned");

} // namespace apdfs
