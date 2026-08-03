#include "core/Bitset.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using namespace apdfs;

TEST(TestBitset, SetAndTest_AllBits)
{
    DynamicBitset bs(128);

    EXPECT_FALSE(bs.test(0));
    EXPECT_FALSE(bs.test(64));
    EXPECT_FALSE(bs.test(127));

    bs.set(0);
    bs.set(64);
    bs.set(127);

    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(64));
    EXPECT_TRUE(bs.test(127));
    EXPECT_FALSE(bs.test(1));
    EXPECT_FALSE(bs.test(63));
    EXPECT_FALSE(bs.test(65));
}

TEST(TestBitset, Reset_SingleBit)
{
    DynamicBitset bs(128);
    bs.set(42);
    ASSERT_TRUE(bs.test(42));

    bs.reset(42);
    EXPECT_FALSE(bs.test(42));
}

TEST(TestBitset, ResetAll_ClearsEverything)
{
    DynamicBitset bs(1000);

    for (size_t i = 0; i < 1000; i += 3)
    {
        bs.set(i);
    }

    bs.resetAll();

    for (size_t i = 0; i < 1000; ++i)
    {
        EXPECT_FALSE(bs.test(i)) << "Bit " << i << " should be cleared";
    }
}

TEST(TestBitset, SetAll_SetsAllBits)
{
    DynamicBitset bs(70);

    bs.setAll();

    for (size_t i = 0; i < 70; ++i)
    {
        EXPECT_TRUE(bs.test(i)) << "Bit " << i << " should be set";
    }
}

TEST(TestBitset, SetAll_PaddingBitsAreZero)
{
    DynamicBitset bs(70);
    bs.setAll();
    bs.resetAll();
    bs.setAll();

    size_t maxSeen = 0;
    bs.forEachSet([&](size_t idx) {
        EXPECT_LT(idx, 70U);
        if (idx > maxSeen)
        {
            maxSeen = idx;
        }
    });
    EXPECT_EQ(maxSeen, 69U);
}

TEST(TestBitset, ForEachSet_NonEmpty)
{
    DynamicBitset bs(256);
    std::vector<size_t> expected = {0, 63, 64, 127, 128, 191, 192, 255};

    for (auto idx : expected)
    {
        bs.set(idx);
    }

    std::vector<size_t> actual;
    bs.forEachSet([&](size_t idx) { actual.push_back(idx); });

    EXPECT_EQ(actual, expected);
}

TEST(TestBitset, ForEachSet_EmptySet)
{
    DynamicBitset bs(256);
    bool called = false;
    bs.forEachSet([&](size_t) { called = true; });
    EXPECT_FALSE(called);
}

TEST(TestBitset, ForEachSet_AllSet)
{
    DynamicBitset bs(128);
    bs.setAll();

    size_t count = 0;
    bs.forEachSet([&](size_t idx) {
        EXPECT_LT(idx, 128U);
        count++;
    });
    EXPECT_EQ(count, 128U);
}

TEST(TestBitset, CopyConstructor)
{
    DynamicBitset src(128);
    src.set(10);
    src.set(100);

    DynamicBitset dst(src);

    EXPECT_TRUE(dst.test(10));
    EXPECT_TRUE(dst.test(100));
    EXPECT_FALSE(dst.test(0));
    EXPECT_EQ(dst.size(), 128U);
}

TEST(TestBitset, CopyAssignment)
{
    DynamicBitset src(128);
    src.set(42);

    DynamicBitset dst(256);
    dst = src;

    EXPECT_EQ(dst.size(), 128U);
    EXPECT_TRUE(dst.test(42));
}

TEST(TestBitset, CopyFrom_SameSize)
{
    DynamicBitset src(128);
    DynamicBitset dst(128);

    src.set(10);
    src.set(100);

    dst.copyFrom(src);

    EXPECT_TRUE(dst.test(10));
    EXPECT_TRUE(dst.test(100));
    EXPECT_FALSE(dst.test(0));
}

TEST(TestBitset, CopyFrom_DifferentSizeIsNoop)
{
    DynamicBitset src(128);
    DynamicBitset dst(256);

    dst.set(200);
    dst.copyFrom(src);

    EXPECT_TRUE(dst.test(200));
}

TEST(TestBitset, MoveConstructor)
{
    DynamicBitset src(128);
    src.set(42);

    DynamicBitset dst(std::move(src));

    EXPECT_TRUE(dst.test(42));
    EXPECT_EQ(dst.size(), 128U);
}

TEST(TestBitset, MoveAssignment)
{
    DynamicBitset src(128);
    src.set(99);

    DynamicBitset dst(256);
    dst = std::move(src);

    EXPECT_EQ(dst.size(), 128U);
    EXPECT_TRUE(dst.test(99));
}

TEST(TestBitset, DataIsAligned)
{
    DynamicBitset bs(64);
    auto ptr = reinterpret_cast<uintptr_t>(bs.data());
    EXPECT_EQ(ptr % DynamicBitset::ALIGNMENT, 0U);
}

TEST(TestBitset, WordBoundary_Bits63and64)
{
    DynamicBitset bs(128);

    bs.set(63);
    EXPECT_TRUE(bs.test(63));

    bs.set(64);
    EXPECT_TRUE(bs.test(64));

    bs.reset(63);
    EXPECT_FALSE(bs.test(63));
    EXPECT_TRUE(bs.test(64));
}

TEST(TestBitset, SingleBitSet)
{
    DynamicBitset bs(1);
    EXPECT_FALSE(bs.test(0));
    bs.set(0);
    EXPECT_TRUE(bs.test(0));
    bs.reset(0);
    EXPECT_FALSE(bs.test(0));
}

TEST(TestBitset, ExactlyOneWord)
{
    DynamicBitset bs(64);

    bs.set(0);
    bs.set(63);

    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(63));

    bs.setAll();
    for (size_t i = 0; i < 64; ++i)
    {
        EXPECT_TRUE(bs.test(i));
    }
}

TEST(TestBitset, LargeBitset)
{
    constexpr size_t N = 1 << 20;
    DynamicBitset bs(N);

    bs.set(0);
    bs.set(N - 1);
    bs.set(N / 2);

    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(N - 1));
    EXPECT_TRUE(bs.test(N / 2));
    EXPECT_FALSE(bs.test(N / 3));

    bs.resetAll();
    EXPECT_FALSE(bs.test(0));
    EXPECT_FALSE(bs.test(N / 2));
}