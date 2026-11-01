#include "core/Types.hpp"

#include <gtest/gtest.h>

#include <unordered_set>

using namespace apdfs;

TEST(TestTypes, Uint128Equality)
{
    Uint128T a{1, 2};
    Uint128T b{1, 2};
    Uint128T c{2, 1};

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(TestTypes, Uint128Xor)
{
    Uint128T a{1, 2};
    Uint128T b{3, 4};
    Uint128T expected;
    expected.lo = 1 xor 3;
    expected.hi = 2 xor 4;
    EXPECT_EQ(a ^ b, expected);
}

TEST(TestTypes, Uint128XorAssign)
{
    Uint128T a{1, 2};
    a ^= Uint128T{3, 4};

    Uint128T expected;
    expected.lo = 1 xor 3;
    expected.hi = 2 xor 4;
    EXPECT_EQ(a, expected);
}

TEST(TestTypes, Uint128XorInverse)
{
    Uint128T a{0xDEADBEEF, 0xCAFEBABE};
    Uint128T b{0x12345678, 0x9ABCDEF0};

    Uint128T c = a ^ b;
    EXPECT_EQ(c ^ b, a);
    EXPECT_EQ(c ^ a, b);
}

TEST(TestTypes, Uint128Hash)
{
    std::unordered_set<Uint128T> s;
    s.insert(Uint128T{0, 0});
    s.insert(Uint128T{1, 2});
    s.insert(Uint128T{1, 2});

    EXPECT_EQ(s.size(), 2);
}

TEST(TestTypes, NullHash)
{
    EXPECT_EQ(NULL_HASH.lo, 0);
    EXPECT_EQ(NULL_HASH.hi, 0);
}

TEST(TestTypes, Alignment)
{
    EXPECT_EQ(alignof(Uint128T), 16);
    EXPECT_EQ(sizeof(Uint128T), 16);
}

TEST(TestTypes, Uint128Xor_Basic)
{
    Uint128T a{1, 2};
    Uint128T b{3, 4};
    Uint128T expected;
    expected.lo = 1 xor 3;
    expected.hi = 2 xor 4;
    EXPECT_EQ(a ^ b, expected);
}

TEST(TestTypes, Uint128Xor_Assignment)
{
    Uint128T a{1, 2};
    Uint128T expected;
    expected.lo = 1 xor 3;
    expected.hi = 2 xor 4;
    a ^= Uint128T{3, 4};
    EXPECT_EQ(a, expected);
}

TEST(TestTypes, Uint128Xor_Inverse)
{
    Uint128T a{0xDEADBEEFCAFEBABEULL, 0x123456789ABCDEF0ULL};
    Uint128T b{0xF00DBABE1234ABCDULL, 0xDEADBEEF87654321ULL};
    Uint128T c = a ^ b;
    EXPECT_EQ(c ^ b, a);
    EXPECT_EQ(c ^ a, b);
}

TEST(TestTypes, Uint128Xor_NullHashIdentity)
{
    Uint128T a;
    a.lo = 42;
    a.hi = 99;
    EXPECT_EQ(a ^ NULL_HASH, a);
}
