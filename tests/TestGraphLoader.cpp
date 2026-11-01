#include "graph/GraphLoader.hpp"

#include <gtest/gtest.h>

#include <fstream>

using namespace apdfs;

TEST(TestGraphLoader, LoadSimpleGraph)
{
    std::filesystem::path tmpFile = "outdir/apdfs_test_graph.txt";
    {
        std::ofstream f(tmpFile);
        f << "3 2\n";
        f << "0 1\n";
        f << "1 2\n";
    }

    GraphLoader::Config cfg;
    cfg.filePath = tmpFile;
    cfg.superSource = 0;
    cfg.superSink = 2;

    auto g = GraphLoader::loadFromFile(cfg);
    ASSERT_TRUE(g.has_value());

    EXPECT_EQ(g->vertexCount(), 5U);

    EXPECT_EQ(g->edgeCount(), 4U);
    EXPECT_EQ(g->superSource(), 3U);
    EXPECT_EQ(g->superSink(), 4U);

    std::filesystem::remove(tmpFile);
}

TEST(TestGraphLoader, LoadGraphWithComments)
{
    std::filesystem::path tmpFile = "outdir/apdfs_test_graph2.txt";
    {
        std::ofstream f(tmpFile);
        f << "# Test graph\n";
        f << "2 1\n";
        f << "0 1 # edge from 0 to 1\n";
    }

    GraphLoader::Config cfg;
    cfg.filePath = tmpFile;
    cfg.superSource = 0;
    cfg.superSink = 1;

    auto g = GraphLoader::loadFromFile(cfg);
    ASSERT_TRUE(g.has_value());
    EXPECT_EQ(g->edgeCount(), 3U);

    std::filesystem::remove(tmpFile);
}

TEST(TestGraphLoader, FileNotFound)
{
    GraphLoader::Config cfg;
    cfg.filePath = "/nonexistent/graph.txt";
    cfg.superSource = 0;
    cfg.superSink = 1;

    auto g = GraphLoader::loadFromFile(cfg);
    EXPECT_FALSE(g.has_value());
}
