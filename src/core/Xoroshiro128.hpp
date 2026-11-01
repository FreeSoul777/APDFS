#pragma once

#include <cstdint>
#include <limits>

namespace apdfs
{

class Xoroshiro128
{
  public:
    using ResultType = uint64_t;

    static constexpr ResultType DEFAULT_SEED = 1234567890123456789ULL;

    explicit Xoroshiro128(ResultType seed = DEFAULT_SEED) noexcept
    {
        mState[0] = splitMix64(seed);
        mState[1] = splitMix64(mState[0]);
    }

    Xoroshiro128(ResultType seed0, ResultType seed1) noexcept : mState{seed0, seed1}
    {
    }

    ResultType operator()() noexcept
    {
        const uint64_t s0 = mState[0];
        uint64_t s1 = mState[1];
        const uint64_t result = rotl(s0 + s1, 17) + s0;

        s1 ^= s0;
        mState[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21);
        mState[1] = rotl(s1, 28);

        return result;
    }

    void discard(unsigned long long count) noexcept
    {
        for (unsigned long long i = 0; i < count; ++i)
        {
            (*this)();
        }
    }

    static constexpr ResultType min() noexcept
    {
        return std::numeric_limits<ResultType>::min();
    }

    static constexpr ResultType max() noexcept
    {
        return std::numeric_limits<ResultType>::max();
    }

  private:
    uint64_t mState[2];

    static uint64_t rotl(uint64_t x, int k) noexcept
    {
        return (x << k) | (x >> (64 - k));
    }

    static uint64_t splitMix64(uint64_t& state) noexcept
    {
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};

} // namespace apdfs
