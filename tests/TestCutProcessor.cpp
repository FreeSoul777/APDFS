#include "cut/BfsEngine.hpp"
#include "cut/CutProcessor.hpp"
#include "cut/SeenTable.hpp"
#include "graph/GraphBuilder.hpp"

#include <gtest/gtest.h>

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

} // namespace

TEST(TestCutProcessor, ProcessFrame_ValidCut_ReturnsCleanCut)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());
    SeenTable seen;
    CutProcessor processor(bfs, g, seen);

    Hash hInit{0, 0};
    hInit ^= g.edgeHash(0);

    Frame frame{hInit, {0}, 0};

    std::vector<EdgeIndex> pClean;
    auto children = processor.processFrame(frame, pClean);

    ASSERT_EQ(pClean.size(), 1U);
    EXPECT_EQ(pClean[0], 0U);

    ASSERT_EQ(children.size(), 1U);

    EXPECT_EQ(children[0].cutEdges.size(), 1U);
    EXPECT_EQ(children[0].cutEdges[0], 1U);
    EXPECT_EQ(children[0].iterator, 0U);

    Hash expectedHash{0, 0};
    expectedHash ^= g.edgeHash(1);
    EXPECT_EQ(children[0].cutHash, expectedHash);
}

TEST(TestCutProcessor, ProcessFrame_InvalidCut_ReturnsEmpty)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());
    SeenTable seen;
    CutProcessor processor(bfs, g, seen);

    Hash hEmpty{0, 0};
    Frame frame{hEmpty, {}, 0};

    std::vector<EdgeIndex> pClean;
    auto children = processor.processFrame(frame, pClean);

    EXPECT_TRUE(pClean.empty());
    EXPECT_TRUE(children.empty());
}

TEST(TestCutProcessor, ProcessFrame_BranchOnLastEdge_NoChildren)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());
    SeenTable seen;
    CutProcessor processor(bfs, g, seen);

    Hash h{0, 0};
    h ^= g.edgeHash(1);

    Frame frame{h, {1}, 0};

    std::vector<EdgeIndex> pClean;
    auto children = processor.processFrame(frame, pClean);

    ASSERT_EQ(pClean.size(), 1U);
    EXPECT_EQ(pClean[0], 1U);

    EXPECT_TRUE(children.empty());
}

TEST(TestCutProcessor, ProcessFrame_IteratorSkipsProcessedEdges)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());
    SeenTable seen;
    CutProcessor processor(bfs, g, seen);

    Hash h{0, 0};
    h ^= g.edgeHash(0);
    Frame frame{h, {0}, 1};

    std::vector<EdgeIndex> pClean;
    auto children = processor.processFrame(frame, pClean);

    EXPECT_EQ(pClean.size(), 1U);
    EXPECT_TRUE(children.empty());
}

TEST(TestCutProcessor, DuplicatePrevention_SeenContains)
{
    auto g = buildPath3();
    BfsEngine bfs(g.vertexCount(), g.edgeCount());
    SeenTable seen;
    CutProcessor processor(bfs, g, seen);

    Hash hChild{0, 0};
    hChild ^= g.edgeHash(1);
    seen.tryMarkProcessing(hChild);

    Hash hInit{0, 0};
    hInit ^= g.edgeHash(0);
    Frame frame{hInit, {0}, 0};

    std::vector<EdgeIndex> pClean;
    auto children = processor.processFrame(frame, pClean);

    EXPECT_TRUE(children.empty());
}
