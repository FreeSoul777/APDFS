#include "app/Options.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace apdfs;

namespace
{

OptionsResult parse(std::vector<std::string> args)
{
    args.insert(args.begin(), "apdfs");
    std::vector<const char*> argv;
    argv.reserve(args.size());
    for (const auto& arg : args)
    {
        argv.push_back(arg.c_str());
    }
    return parseOptions(static_cast<int>(argv.size()), argv.data());
}

} // namespace

TEST(TestCli, DefaultsWithRequiredArgumentsOnly)
{
    const auto result = parse({"graph.txt", "0", "7"});
    ASSERT_TRUE(result.ok()) << result.error;

    EXPECT_EQ(result.options.graphFile, "graph.txt");
    EXPECT_EQ(result.options.source, 0U);
    EXPECT_EQ(result.options.sink, 7U);
    EXPECT_GE(result.options.threads, 1U);
    EXPECT_EQ(result.options.maxCuts, 0U);
    EXPECT_EQ(result.options.maxExamined, 0U);
    EXPECT_EQ(result.options.maxCutSize, 0U);
    EXPECT_FALSE(result.options.showProgress);
}

TEST(TestCli, PositionalThreadsAndOutput)
{
    const auto result = parse({"graph.txt", "0", "7", "4", "out"});
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.options.threads, 4U);
    EXPECT_EQ(result.options.outputDir, "out");
}

TEST(TestCli, LongOptionsOverridePositionals)
{
    const auto result = parse({"graph.txt", "0", "7", "2", "out", "--threads=8", "--output=dir"});
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.options.threads, 8U);
    EXPECT_EQ(result.options.outputDir, "dir");
}

TEST(TestCli, LimitsAndProgress)
{
    const auto result =
        parse({"graph.txt", "0", "7", "--max-cuts=5", "--max-cut-size=3", "--max-examined=9", "--progress"});
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.options.maxCuts, 5U);
    EXPECT_EQ(result.options.maxCutSize, 3U);
    EXPECT_EQ(result.options.maxExamined, 9U);
    EXPECT_TRUE(result.options.showProgress);
}

TEST(TestCli, ZeroLimitsMeanUnlimited)
{
    const auto result = parse({"graph.txt", "0", "7", "--max-cuts=0", "--max-examined=0", "--max-cut-size=0"});
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.options.maxCuts, 0U);
    EXPECT_EQ(result.options.maxExamined, 0U);
    EXPECT_EQ(result.options.maxCutSize, 0U);
}

TEST(TestCli, UnknownOptionIsRejected)
{
    const auto result = parse({"graph.txt", "0", "7", "--nonsense=1"});
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error.find("unknown option"), std::string::npos) << result.error;
}

TEST(TestCli, MissingValueIsRejected)
{
    const auto result = parse({"graph.txt", "0", "7", "--max-cuts"});
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error.find("requires a value"), std::string::npos) << result.error;
}

TEST(TestCli, NonNumericValuesAreRejected)
{
    EXPECT_FALSE(parse({"graph.txt", "0", "7", "--max-cuts=abc"}).ok());
    EXPECT_FALSE(parse({"graph.txt", "0", "7", "--threads=0"}).ok());
    EXPECT_FALSE(parse({"graph.txt", "0", "7", "x"}).ok());
    EXPECT_FALSE(parse({"graph.txt", "x", "7"}).ok());
}

TEST(TestCli, SourceSinkMustFitVertexIndex)
{
    const auto tooBig = parse({"graph.txt", "4294967296", "7"});
    EXPECT_FALSE(tooBig.ok());
    EXPECT_NE(tooBig.error.find("invalid source"), std::string::npos) << tooBig.error;

    const auto maxOk = parse({"graph.txt", "4294967295", "7"});
    EXPECT_TRUE(maxOk.ok()) << maxOk.error;
    EXPECT_EQ(maxOk.options.source, 4294967295U);
}

TEST(TestCli, MissingRequiredArguments)
{
    EXPECT_FALSE(parse({}).ok());
    EXPECT_FALSE(parse({"graph.txt"}).ok());
    EXPECT_FALSE(parse({"graph.txt", "0"}).ok());
}

TEST(TestCli, TooManyPositionalArguments)
{
    const auto result = parse({"graph.txt", "0", "7", "4", "out", "extra"});
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.error.find("too many"), std::string::npos) << result.error;
}

TEST(TestCli, HelpAndVersionDoNotRequireArguments)
{
    const auto help = parse({"--help"});
    ASSERT_TRUE(help.ok()) << help.error;
    EXPECT_TRUE(help.options.help);

    const auto version = parse({"--version"});
    ASSERT_TRUE(version.ok()) << version.error;
    EXPECT_TRUE(version.options.version);

    EXPECT_FALSE(usageText().empty());
}
