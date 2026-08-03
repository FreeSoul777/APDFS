#include "cut/ApdfsSolver.hpp"
#include "graph/GraphBuilder.hpp"
#include "graph/GraphLoader.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace apdfs;

TEST(TestIntegration, EndToEnd_WriteAndReadBack)
{
    std::filesystem::path graphFile = "outdir/apdfs_integration_graph.txt";
    {
        std::ofstream f(graphFile);
        f << "2 1\n";
        f << "0 1\n";
    }

    GraphLoader::Config loaderCfg;
    loaderCfg.filePath = graphFile;
    loaderCfg.superSource = 0;
    loaderCfg.superSink = 1;
    auto g = GraphLoader::loadFromFile(loaderCfg);
    ASSERT_TRUE(g.has_value());

    std::filesystem::path outputDir = "outdir/apdfs_integration_output";
    std::filesystem::remove_all(outputDir);

    SolverConfig cfg;
    cfg.outputDir = outputDir;
    cfg.collectInMemory = true;
    cfg.showProgress = false;

    ApdfsSolver solver(*g, cfg);
    solver.run();

    EXPECT_TRUE(std::filesystem::exists(outputDir));

    size_t fileCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(outputDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".bin")
        {
            fileCount++;
        }
    }
    EXPECT_GT(fileCount, 0U);

    const auto& cuts = solver.getCuts();
    EXPECT_FALSE(cuts.empty());

    for (const auto& cut : cuts)
    {
        EXPECT_FALSE(cut.empty());
    }

    std::filesystem::remove(graphFile);
    std::filesystem::remove_all(outputDir);
}

TEST(TestIntegration, DisconnectedGraph_ReturnsEmpty)
{
    std::filesystem::path graphFile = "outdir/apdfs_integration_disconnected.txt";
    {
        std::ofstream f(graphFile);
        f << "3 1\n";
        f << "0 1\n";
    }

    GraphLoader::Config loaderCfg;
    loaderCfg.filePath = graphFile;
    loaderCfg.superSource = 0;
    loaderCfg.superSink = 2;
    auto g = GraphLoader::loadFromFile(loaderCfg);
    ASSERT_TRUE(g.has_value());

    std::filesystem::path outputDir = "outdir/apdfs_integration_disconnected_out";
    std::filesystem::remove_all(outputDir);

    SolverConfig cfg;
    cfg.outputDir = outputDir;
    cfg.collectInMemory = true;

    ApdfsSolver solver(*g, cfg);
    solver.run();

    EXPECT_TRUE(solver.getCuts().empty());

    std::filesystem::remove(graphFile);
    std::filesystem::remove_all(outputDir);
}
