#include "cut/BfsEngine.hpp"
#include "graph/GraphBuilder.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using namespace apdfs;

namespace
{

Graph buildPath3()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(2);
    builder.addEdge(0, 1);
    builder.addEdge(1, 2);
    return Graph(std::move(builder));
}

Graph buildDiamond()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(3);
    builder.addEdges({{0, 1}, {0, 2}, {1, 3}, {2, 3}});
    return Graph(std::move(builder));
}

} // namespace

TEST(TestBfsEngine, ForwardBfs_SourceInV1)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    std::vector<EdgeIndex> cutEdges;

    bool reachedSink = bfs.forwardBfs(0, 2, g, forbidden, cutEdges);

    EXPECT_TRUE(bfs.v1Mask().test(0));
    EXPECT_TRUE(reachedSink);
}

TEST(TestBfsEngine, ForwardBfs_AllReachableWithoutForbidden)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    std::vector<EdgeIndex> cutEdges;

    bfs.forwardBfs(0, 2, g, forbidden, cutEdges);

    EXPECT_TRUE(bfs.v1Mask().test(0));
    EXPECT_TRUE(bfs.v1Mask().test(1));
    EXPECT_TRUE(bfs.v1Mask().test(2));
}

TEST(TestBfsEngine, ForwardBfs_ForbiddenEdgeBlocksPath)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    forbidden.set(0);
    std::vector<EdgeIndex> cutEdges;

    bool reachedSink = bfs.forwardBfs(0, 2, g, forbidden, cutEdges);

    EXPECT_FALSE(reachedSink);
    EXPECT_TRUE(bfs.v1Mask().test(0));
    EXPECT_FALSE(bfs.v1Mask().test(1));
    EXPECT_FALSE(bfs.v1Mask().test(2));
}

TEST(TestBfsEngine, ForwardBfs_CutEdgesAreCorrect)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    forbidden.set(0);
    std::vector<EdgeIndex> cutEdges;

    bfs.forwardBfs(0, 2, g, forbidden, cutEdges);

    ASSERT_EQ(cutEdges.size(), 1U);
    EXPECT_EQ(cutEdges[0], 0U);
}

TEST(TestBfsEngine, ForwardBfs_CutEdgesCrossBoundary)
{
    auto g = buildDiamond();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    forbidden.set(0);
    forbidden.set(1);
    std::vector<EdgeIndex> cutEdges;

    bfs.forwardBfs(0, 3, g, forbidden, cutEdges);

    ASSERT_EQ(cutEdges.size(), 2U);

    for (auto e : cutEdges)
    {
        VertexIndex u = g.edgeSource(e);
        VertexIndex w = g.edgeTarget(e);
        EXPECT_TRUE(bfs.v1Mask().test(u));
        EXPECT_FALSE(bfs.v1Mask().test(w));
    }
}

TEST(TestBfsEngine, IncrementalForwardBfs_FromVertexAdded)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden1(g.edgeCount());
    forbidden1.set(0);
    std::vector<EdgeIndex> cutEdges;
    bfs.forwardBfs(0, 2, g, forbidden1, cutEdges);

    DynamicBitset forbidden2(g.edgeCount());

    forbidden2.set(1);

    bool reached = bfs.incrementalForwardBfs(1, 2, g, forbidden2);

    EXPECT_TRUE(bfs.v1Mask().test(1));
    EXPECT_FALSE(reached);
}

TEST(TestBfsEngine, IncrementalForwardBfs_ReachesSink)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden1(g.edgeCount());
    forbidden1.set(0);
    std::vector<EdgeIndex> cutEdges;
    bfs.forwardBfs(0, 2, g, forbidden1, cutEdges);

    DynamicBitset forbidden2(g.edgeCount());

    bool reached = bfs.incrementalForwardBfs(1, 2, g, forbidden2);

    EXPECT_TRUE(reached);
    EXPECT_TRUE(bfs.v1Mask().test(2));
}

TEST(TestBfsEngine, IncrementalForwardBfs_CandidatesCollected)
{
    auto g = buildDiamond();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden1(g.edgeCount());
    forbidden1.set(0);
    std::vector<EdgeIndex> cutEdges;
    bfs.forwardBfs(0, 3, g, forbidden1, cutEdges);

    DynamicBitset forbidden2(g.edgeCount());
    forbidden2.set(2);

    bfs.incrementalForwardBfs(1, 3, g, forbidden2);

    EXPECT_TRUE(bfs.v1Mask().test(0));
    EXPECT_TRUE(bfs.v1Mask().test(1));
}

TEST(TestBfsEngine, BackwardBfs_SinkInVT)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    bfs.backwardBfs(2, g, forbidden);

    EXPECT_TRUE(bfs.vtMask().test(2));
}

TEST(TestBfsEngine, BackwardBfs_AllReachableWithoutForbidden)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    bfs.backwardBfs(2, g, forbidden);

    EXPECT_TRUE(bfs.vtMask().test(0));
    EXPECT_TRUE(bfs.vtMask().test(1));
    EXPECT_TRUE(bfs.vtMask().test(2));
}

TEST(TestBfsEngine, BackwardBfs_ForbiddenEdgeBlocks)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    forbidden.set(1);
    bfs.backwardBfs(2, g, forbidden);

    EXPECT_TRUE(bfs.vtMask().test(2));
    EXPECT_FALSE(bfs.vtMask().test(1));
    EXPECT_FALSE(bfs.vtMask().test(0));
}

TEST(TestBfsEngine, FilterByVT_KeepsValidEdges)
{
    auto g = buildDiamond();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset v1(g.vertexCount());
    DynamicBitset vt(g.vertexCount());
    v1.set(0);
    v1.set(1);
    vt.set(3);

    std::vector<EdgeIndex> candidates = {0, 1, 2, 3};
    std::vector<EdgeIndex> result;

    bfs.filterByVT(g, v1, vt, candidates, result);

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0], 2U);
}

TEST(TestBfsEngine, FilterByVT_RemovesEdgesNotInV1)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset v1(g.vertexCount());
    DynamicBitset vt(g.vertexCount());
    v1.set(0);
    vt.set(1);
    vt.set(2);

    std::vector<EdgeIndex> candidates = {0, 1};
    std::vector<EdgeIndex> result;

    bfs.filterByVT(g, v1, vt, candidates, result);

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0], 0U);
}

TEST(TestBfsEngine, FilterByVT_RemovesEdgesNotInVT)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset v1(g.vertexCount());
    DynamicBitset vt(g.vertexCount());
    v1.set(0);
    v1.set(1);
    vt.set(2);

    std::vector<EdgeIndex> candidates = {0, 1};
    std::vector<EdgeIndex> result;

    bfs.filterByVT(g, v1, vt, candidates, result);

    ASSERT_EQ(result.size(), 1U);
    EXPECT_EQ(result[0], 1U);
}

TEST(TestBfsEngine, Integration_ForwardBackwardFilter)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());

    DynamicBitset forbidden(g.edgeCount());
    forbidden.set(0);
    std::vector<EdgeIndex> pActual;
    bfs.forwardBfs(0, 2, g, forbidden, pActual);

    DynamicBitset qMask(g.edgeCount());
    qMask.set(1);
    bfs.incrementalForwardBfs(1, 2, g, qMask);

    std::vector<EdgeIndex> candidates;
    for (VertexIndex u = 0; u < static_cast<VertexIndex>(g.vertexCount()); ++u)
    {
        if (!bfs.v1Mask().test(u))
        {
            continue;
        }
        for (EdgeIndex e : g.outgoingEdges(u))
        {
            VertexIndex w = g.edgeTarget(e);
            if (!bfs.v1Mask().test(w))
            {
                candidates.push_back(e);
            }
        }
    }

    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates[0], 1U);

    DynamicBitset candMask(g.edgeCount());
    candMask.set(1);
    bfs.backwardBfs(2, g, candMask);

    std::vector<EdgeIndex> pNew;
    bfs.filterByVT(g, bfs.v1Mask(), bfs.vtMask(), candidates, pNew);

    ASSERT_EQ(pNew.size(), 1U);
    EXPECT_EQ(pNew[0], 1U);
}
