#pragma once

#include <cstdint>
#include <functional>

namespace apdfs
{

using VertexIndex = uint32_t;
using EdgeIndex = uint32_t;

struct alignas(16) Uint128T
{
    uint64_t lo;
    uint64_t hi;

    bool operator==(const Uint128T& other) const noexcept
    {
        return lo == other.lo && hi == other.hi;
    }

    bool operator!=(const Uint128T& other) const noexcept
    {
        return !(*this == other);
    }

    Uint128T& operator^=(const Uint128T& rhs) noexcept
    {
        lo ^= rhs.lo;
        hi ^= rhs.hi;
        return *this;
    }

    Uint128T operator^(const Uint128T& rhs) const noexcept
    {
        Uint128T result = *this;
        result ^= rhs;
        return result;
    }
};

static_assert(sizeof(Uint128T) == 16, "uint128_t must be 16 bytes");
static_assert(alignof(Uint128T) == 16, "uint128_t must be 16-byte aligned");

using Hash = Uint128T;

constexpr Hash NULL_HASH{0, 0};

} // namespace apdfs

namespace std
{

template <>
struct hash<apdfs::Uint128T>
{
    size_t operator()(const apdfs::Uint128T& val) const noexcept
    {
        return val.lo ^ (val.hi >> 32) ^ (val.hi << 16);
    }
};

} // namespace std
