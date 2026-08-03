#include "cut/SeenTable.hpp"

#include <gtest/gtest.h>

#include <unordered_set>

using namespace apdfs;

TEST(TestSeenTable, TryMarkProcessing_NewHash_ReturnsTrue)
{
    SeenTable table;
    Hash h{0x1234, 0x5678};

    EXPECT_TRUE(table.tryMarkProcessing(h));
}

TEST(TestSeenTable, TryMarkProcessing_SameHashTwice_ReturnsFalse)
{
    SeenTable table;
    Hash h{0xAAAA, 0xBBBB};

    ASSERT_TRUE(table.tryMarkProcessing(h));
    EXPECT_FALSE(table.tryMarkProcessing(h));
}

TEST(TestSeenTable, Contains_AfterInsert_ReturnsTrue)
{
    SeenTable table;
    Hash h{0xDEAD, 0xBEEF};

    table.tryMarkProcessing(h);
    EXPECT_TRUE(table.contains(h));
}

TEST(TestSeenTable, Contains_BeforeInsert_ReturnsFalse)
{
    SeenTable table;
    Hash h{0xCAFE, 0xBABE};

    EXPECT_FALSE(table.contains(h));
}

TEST(TestSeenTable, Contains_DifferentHash_ReturnsFalse)
{
    SeenTable table;
    Hash h1{1, 1};
    Hash h2{2, 2};

    table.tryMarkProcessing(h1);
    EXPECT_FALSE(table.contains(h2));
}

TEST(TestSeenTable, MarkDone_HashStillContained)
{
    SeenTable table;
    Hash h{0xF00D, 0x1234};

    table.tryMarkProcessing(h);
    table.markDone(h);
    EXPECT_TRUE(table.contains(h));
}

TEST(TestSeenTable, MarkDone_NonExistent_DoesNotCrash)
{
    SeenTable table;
    Hash h{0x9999, 0x8888};

    table.markDone(h);
    SUCCEED();
}

TEST(TestSeenTable, MultipleUniqueHashes_AllInserted)
{
    SeenTable table;

    for (uint64_t i = 0; i < 1000; ++i)
    {
        Hash h{i, i * 2};
        EXPECT_TRUE(table.tryMarkProcessing(h)) << "Failed at hash " << i;
    }
}

TEST(TestSeenTable, MultipleUniqueHashes_AllContained)
{
    SeenTable table;
    std::vector<Hash> inserted;

    for (uint64_t i = 0; i < 500; ++i)
    {
        Hash h{i, i * 3};
        table.tryMarkProcessing(h);
        inserted.push_back(h);
    }

    for (const auto& h : inserted)
    {
        EXPECT_TRUE(table.contains(h));
    }
}

TEST(TestSeenTable, Size_MatchesInsertCount)
{
    SeenTable table;

    for (uint64_t i = 0; i < 100; ++i)
    {
        table.tryMarkProcessing(Hash{i, i});
    }

    EXPECT_EQ(table.size(), 100U);
}

TEST(TestSeenTable, Duplicates_DoNotIncreaseSize)
{
    SeenTable table;

    table.tryMarkProcessing(Hash{42, 42});
    table.tryMarkProcessing(Hash{42, 42});
    table.tryMarkProcessing(Hash{42, 42});

    EXPECT_EQ(table.size(), 1U);
}

TEST(TestSeenTable, TwoDifferentKeys_SameHashDifferentKey_AreDistinct)
{
    SeenTable table;
    Hash h1{0, 1};
    Hash h2{1, 0};

    EXPECT_TRUE(table.tryMarkProcessing(h1));
    EXPECT_TRUE(table.tryMarkProcessing(h2));

    EXPECT_TRUE(table.contains(h1));
    EXPECT_TRUE(table.contains(h2));
}

TEST(TestSeenTable, Stress_10kInsertions)
{
    SeenTable table;
    constexpr size_t N = 10000;

    for (size_t i = 0; i < N; ++i)
    {
        Hash h{i, i * 7 + 13};
        ASSERT_TRUE(table.tryMarkProcessing(h));
    }

    EXPECT_EQ(table.size(), N);

    for (size_t i = 0; i < N; ++i)
    {
        Hash h{i, i * 7 + 13};
        EXPECT_TRUE(table.contains(h));
    }
}

TEST(TestSeenTable, Rehash_TriggeredByLoadFactor)
{
    SeenTable table;

    for (uint64_t i = 0; i < 5000; ++i)
    {
        Hash h{i, i};
        ASSERT_TRUE(table.tryMarkProcessing(h));
    }

    for (uint64_t i = 0; i < 5000; ++i)
    {
        Hash h{i, i};
        EXPECT_TRUE(table.contains(h));
    }
}

TEST(TestSeenTable, ComputeOptimalSegments)
{
    EXPECT_EQ(SeenTable::computeOptimalSegments(1), 1024U);
    EXPECT_EQ(SeenTable::computeOptimalSegments(4), 1024U);
    EXPECT_EQ(SeenTable::computeOptimalSegments(8), 2048U);
    EXPECT_EQ(SeenTable::computeOptimalSegments(16), 4096U);
    EXPECT_EQ(SeenTable::computeOptimalSegments(32), 8192U);
    EXPECT_EQ(SeenTable::computeOptimalSegments(64), 16384U);
}

TEST(TestSeenTable, NumSegments_IsPowerOfTwo)
{
    for (size_t t : {1, 2, 3, 4, 7, 8, 13, 16, 31, 32, 57, 64})
    {
        size_t n = SeenTable::computeOptimalSegments(t);
        EXPECT_EQ(n & (n - 1), 0U) << "t=" << t << " n=" << n;
        EXPECT_GE(n, 1024U);
    }
}
