#include "core/Xoroshiro128.hpp"

#include <gtest/gtest.h>

#include <unordered_set>

using namespace apdfs;

TEST(TestXoroshiro128, Determinism_SameSeed)
{
    Xoroshiro128 rng1(42);
    Xoroshiro128 rng2(42);

    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_EQ(rng1(), rng2());
    }
}

TEST(TestXoroshiro128, Determinism_DifferentSeedsDiverge)
{
    Xoroshiro128 rng1(42);
    Xoroshiro128 rng2(123);

    bool allSame = true;
    for (int i = 0; i < 100; ++i)
    {
        if (rng1() != rng2())
        {
            allSame = false;
            break;
        }
    }
    EXPECT_FALSE(allSame) << "Different seeds must produce different sequences";
}

TEST(TestXoroshiro128, TwoSeedConstructor_ExactState)
{
    Xoroshiro128 rng(0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL);

    uint64_t first = rng();

    Xoroshiro128 rng2(0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL);

    EXPECT_EQ(rng2(), first);
}

TEST(TestXoroshiro128, Determinism_TwoSeedConstructor)
{
    Xoroshiro128 rng1(999);
    rng1.discard(10);

    Xoroshiro128 rng2(999);
    rng2.discard(10);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(rng1(), rng2());
    }
}

TEST(TestXoroshiro128, NoDuplicatesInWindow)
{
    Xoroshiro128 rng(777);
    std::unordered_set<uint64_t> seen;
    seen.reserve(200000);

    for (int i = 0; i < 200000; ++i)
    {
        auto val = rng();
        ASSERT_TRUE(seen.insert(val).second) << "Duplicate at iteration " << i;
    }
}

TEST(TestXoroshiro128, Discard_SkipsCorrectly)
{
    Xoroshiro128 rng1(42);
    Xoroshiro128 rng2(42);

    rng1.discard(100);
    for (int i = 0; i < 100; ++i)
    {
        rng2();
    }

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(rng1(), rng2());
    }
}

TEST(TestXoroshiro128, DefaultSeed_IsDeterministic)
{
    Xoroshiro128 rng1;
    Xoroshiro128 rng2;
    EXPECT_EQ(rng1(), rng2());
}

TEST(TestXoroshiro128, ZeroSeed)
{
    Xoroshiro128 rng1(0);
    Xoroshiro128 rng2(0);
    EXPECT_EQ(rng1(), rng2());
}

TEST(TestXoroshiro128, MaxUint64Seed)
{
    Xoroshiro128 rng(UINT64_MAX);

    rng();
    rng();
    SUCCEED();
}
