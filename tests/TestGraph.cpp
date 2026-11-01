#include "graph/Graph.hpp"
#include "graph/GraphBuilder.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <unordered_set>

using namespace apdfs;

namespace
{

Graph buildSingleEdge()
{
    GraphBuilder builder;
    builder.setSuperSource(0);
    builder.setSuperSink(1);
    builder.addEdge(0, 1);
    return Graph(std::move(builder));
}

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

TEST(TestGraph, VertexCount_SingleEdge)
{
    auto g = buildSingleEdge();
    EXPECT_EQ(g.vertexCount(), 2U);
}

TEST(TestGraph, EdgeCount_SingleEdge)
{
    auto g = buildSingleEdge();
    EXPECT_EQ(g.edgeCount(), 1U);
}

TEST(TestGraph, VertexCount_Path3)
{
    auto g = buildPath3();
    EXPECT_EQ(g.vertexCount(), 3U);
}

TEST(TestGraph, EdgeCount_Path3)
{
    auto g = buildPath3();
    EXPECT_EQ(g.edgeCount(), 2U);
}

TEST(TestGraph, SuperSource_SingleEdge)
{
    auto g = buildSingleEdge();
    EXPECT_EQ(g.superSource(), 0U);
}

TEST(TestGraph, SuperSink_SingleEdge)
{
    auto g = buildSingleEdge();
    EXPECT_EQ(g.superSink(), 1U);
}

TEST(TestGraph, EdgeSource)
{
    auto g = buildPath3();
    EXPECT_EQ(g.edgeSource(0), 0U);
    EXPECT_EQ(g.edgeSource(1), 1U);
}

TEST(TestGraph, EdgeTarget)
{
    auto g = buildPath3();
    EXPECT_EQ(g.edgeTarget(0), 1U);
    EXPECT_EQ(g.edgeTarget(1), 2U);
}

TEST(TestGraph, EdgeHash_Deterministic)
{
    GraphBuilder builder1;
    builder1.setSuperSource(0);
    builder1.setSuperSink(2);
    builder1.addEdge(0, 1);
    builder1.addEdge(1, 2);
    auto g1 = Graph(std::move(builder1));

    GraphBuilder builder2;
    builder2.setSuperSource(0);
    builder2.setSuperSink(2);
    builder2.addEdge(0, 1);
    builder2.addEdge(1, 2);
    auto g2 = Graph(std::move(builder2));

    EXPECT_EQ(g1.edgeHash(0), g2.edgeHash(0));
    EXPECT_EQ(g1.edgeHash(1), g2.edgeHash(1));
}

TEST(TestGraph, EdgeHash_Unique)
{
    auto g = buildDiamond();
    std::unordered_set<Hash> hashes;
    for (EdgeIndex e = 0; e < static_cast<EdgeIndex>(g.edgeCount()); ++e)
    {
        ASSERT_TRUE(hashes.insert(g.edgeHash(e)).second) << "Hash collision at edge " << e;
    }
}

TEST(TestGraph, OutgoingEdges_SingleEdge)
{
    auto g = buildSingleEdge();
    auto out = g.outgoingEdges(0);
    ASSERT_EQ(out.size(), 1U);
    EXPECT_EQ(g.edgeTarget(out[0]), 1U);
}

TEST(TestGraph, OutgoingEdges_Path3)
{
    auto g = buildPath3();

    auto out0 = g.outgoingEdges(0);
    ASSERT_EQ(out0.size(), 1U);
    EXPECT_EQ(g.edgeTarget(out0[0]), 1U);

    auto out1 = g.outgoingEdges(1);
    ASSERT_EQ(out1.size(), 1U);
    EXPECT_EQ(g.edgeTarget(out1[0]), 2U);

    auto out2 = g.outgoingEdges(2);
    EXPECT_EQ(out2.size(), 0U);
}

TEST(TestGraph, OutgoingEdges_Diamond)
{
    auto g = buildDiamond();

    auto out0 = g.outgoingEdges(0);
    ASSERT_EQ(out0.size(), 2U);

    auto out1 = g.outgoingEdges(1);
    ASSERT_EQ(out1.size(), 1U);

    auto out2 = g.outgoingEdges(2);
    ASSERT_EQ(out2.size(), 1U);
}

TEST(TestGraph, OutDegree)
{
    auto g = buildDiamond();
    EXPECT_EQ(g.outDegree(0), 2U);
    EXPECT_EQ(g.outDegree(1), 1U);
    EXPECT_EQ(g.outDegree(3), 0U);
}

TEST(TestGraph, IncomingEdges_SingleEdge)
{
    auto g = buildSingleEdge();
    auto inc = g.incomingEdges(1);
    ASSERT_EQ(inc.size(), 1U);
    EXPECT_EQ(g.edgeSource(inc[0]), 0U);
}

TEST(TestGraph, IncomingEdges_Diamond)
{
    auto g = buildDiamond();

    auto inc3 = g.incomingEdges(3);
    ASSERT_EQ(inc3.size(), 2U);

    auto inc1 = g.incomingEdges(1);
    ASSERT_EQ(inc1.size(), 1U);
    EXPECT_EQ(g.edgeSource(inc1[0]), 0U);
}

TEST(TestGraph, InDegree)
{
    auto g = buildDiamond();
    EXPECT_EQ(g.inDegree(3), 2U);
    EXPECT_EQ(g.inDegree(1), 1U);
    EXPECT_EQ(g.inDegree(0), 0U);
}

TEST(TestGraph, OutgoingEdges_AllEdgesReachable)
{
    auto g = buildDiamond();
    std::unordered_set<EdgeIndex> seen;

    for (VertexIndex v = 0; v < static_cast<VertexIndex>(g.vertexCount()); ++v)
    {
        for (auto e : g.outgoingEdges(v))
        {
            seen.insert(e);
        }
    }

    EXPECT_EQ(seen.size(), g.edgeCount());
}

TEST(TestGraph, IncomingEdges_AllEdgesReachable)
{
    auto g = buildDiamond();
    std::unordered_set<EdgeIndex> seen;

    for (VertexIndex v = 0; v < static_cast<VertexIndex>(g.vertexCount()); ++v)
    {
        for (auto e : g.incomingEdges(v))
        {
            seen.insert(e);
        }
    }

    EXPECT_EQ(seen.size(), g.edgeCount());
}

TEST(TestGraph, AutoSuperVertices_WhenNotSet)
{
    GraphBuilder builder;
    builder.addEdge(0, 1);
    builder.addEdge(1, 2);

    Graph g(std::move(builder));

    EXPECT_EQ(g.vertexCount(), 5U);
    EXPECT_EQ(g.superSource(), 3U);
    EXPECT_EQ(g.superSink(), 4U);
    EXPECT_EQ(g.outDegree(g.superSource()), 0U);
    EXPECT_EQ(g.inDegree(g.superSink()), 0U);
}
