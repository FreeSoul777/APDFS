#include "TestOracles.hpp"

#include "cut/CutWriter.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace apdfs;

namespace
{

constexpr uint32_t MAGIC = 0x4544484D;
constexpr uint32_t VERSION = 1;

std::filesystem::path freshDir(const std::string& name)
{
    auto dir = std::filesystem::path("outdir") / name;
    std::filesystem::remove_all(dir);
    return dir;
}

std::vector<EdgeIndex> makeCut(size_t size, EdgeIndex offset = 0)
{
    std::vector<EdgeIndex> cut(size);
    for (size_t i = 0; i < size; ++i)
    {
        cut[i] = offset + static_cast<EdgeIndex>(i);
    }
    return cut;
}

} // namespace

TEST(TestCutWriter, WritesHeaderAndRecords)
{
    const auto dir = freshDir("cutwriter_basic");
    {
        CutWriter writer(dir, 7);
        writer.writeCut(std::vector<EdgeIndex>{0});
        writer.writeCut(std::vector<EdgeIndex>{2, 3});
        writer.writeCut(std::vector<EdgeIndex>{1, 2, 3});
        writer.flush();

        EXPECT_EQ(writer.totalBytesWritten(), 52U);
    }

    auto files = test::listCutFiles(dir);
    ASSERT_EQ(files.size(), 1U);

    auto file = test::readCutFile(files.front());
    EXPECT_EQ(file.magic, MAGIC);
    EXPECT_EQ(file.version, VERSION);
    ASSERT_EQ(file.records.size(), 3U);
    EXPECT_EQ(file.records[0].edges, (std::vector<uint32_t>{0}));
    EXPECT_EQ(file.records[1].edges, (std::vector<uint32_t>{2, 3}));
    EXPECT_EQ(file.records[2].edges, (std::vector<uint32_t>{1, 2, 3}));

    std::filesystem::remove_all(dir);
}

TEST(TestCutWriter, FileNameCarriesThreadIdAndExtension)
{
    const auto dir = freshDir("cutwriter_name");
    {
        CutWriter writer(dir, 7);
        writer.writeCut(std::vector<EdgeIndex>{0});
        writer.flush();
    }

    auto files = test::listCutFiles(dir);
    ASSERT_EQ(files.size(), 1U);

    const std::string name = files.front().filename().string();
    EXPECT_EQ(name.rfind("cuts_", 0), 0U) << name;
    EXPECT_NE(name.find("_t7_"), std::string::npos) << name;
    EXPECT_EQ(files.front().extension().string(), ".bin");

    std::filesystem::remove_all(dir);
}

TEST(TestCutWriter, RotationAtMaxFileSize)
{
    const auto dir = freshDir("cutwriter_rotation");
    constexpr uint32_t kCutEdges = 8;
    constexpr size_t kCuts = 130000;
    const uint64_t recordBytes = 4 + kCutEdges * 4;

    uint64_t bytesWritten = 0;
    uint32_t rotated = 0;
    {
        CutWriter writer(dir, 0);
        auto cut = makeCut(kCutEdges);
        for (size_t i = 0; i < kCuts; ++i)
        {
            writer.writeCut(cut);
        }
        writer.flush();
        bytesWritten = writer.totalBytesWritten();
        rotated = writer.filesRotated();
    }

    EXPECT_GE(rotated, 1U) << "130k records of " << recordBytes << " bytes must overflow one 4 MiB file";

    auto files = test::listCutFiles(dir);
    ASSERT_GE(files.size(), 2U);
    EXPECT_EQ(files.size(), static_cast<size_t>(rotated) + 1U);

    uint64_t totalOnDisk = 0;
    size_t recordsOnDisk = 0;
    for (const auto& path : files)
    {
        const auto size = static_cast<uint64_t>(std::filesystem::file_size(path));
        EXPECT_LE(size, CutWriter::MAX_FILE_SIZE) << path.filename();
        totalOnDisk += size;

        auto file = test::readCutFile(path);
        EXPECT_EQ(file.magic, MAGIC);
        EXPECT_EQ(file.version, VERSION);
        recordsOnDisk += file.records.size();
    }

    EXPECT_EQ(recordsOnDisk, kCuts);
    EXPECT_EQ(totalOnDisk, 16 * files.size() + kCuts * recordBytes);
    EXPECT_EQ(bytesWritten, totalOnDisk);

    std::filesystem::remove_all(dir);
}

TEST(TestCutWriter, OversizedRecordBypassesBuffer)
{
    const auto dir = freshDir("cutwriter_oversized");
    const size_t edgeCount = 300000;
    const uint64_t recordBytes = 4 + edgeCount * 4;

    {
        CutWriter writer(dir, 0);
        writer.writeCut(makeCut(edgeCount));
        writer.flush();
        EXPECT_EQ(writer.totalBytesWritten(), 16U + recordBytes);
    }

    auto files = test::listCutFiles(dir);
    ASSERT_EQ(files.size(), 1U);
    EXPECT_EQ(static_cast<uint64_t>(std::filesystem::file_size(files.front())), 16U + recordBytes);

    auto file = test::readCutFile(files.front());
    ASSERT_EQ(file.records.size(), 1U);
    EXPECT_EQ(file.records.front().edges.size(), edgeCount);
    EXPECT_EQ(file.records.front().edges.front(), 0U);
    EXPECT_EQ(file.records.front().edges.back(), static_cast<uint32_t>(edgeCount - 1));

    std::filesystem::remove_all(dir);
}

TEST(TestCutWriter, ManyRecordsAcrossBufferBoundaries)
{
    const auto dir = freshDir("cutwriter_boundaries");
    constexpr uint32_t kCuts = 140000;

    {
        CutWriter writer(dir, 1);
        for (uint32_t i = 0; i < kCuts; ++i)
        {
            writer.writeCut(std::vector<EdgeIndex>{i});
        }
        writer.flush();
    }

    auto files = test::listCutFiles(dir);
    ASSERT_FALSE(files.empty());

    uint64_t records = 0;
    EdgeIndex expected = 0;
    for (const auto& path : files)
    {
        auto file = test::readCutFile(path);
        for (const auto& record : file.records)
        {
            ASSERT_EQ(record.edges.size(), 1U);
            ASSERT_EQ(record.edges.front(), expected) << "record order broken at " << records;
            ++expected;
            ++records;
        }
    }

    EXPECT_EQ(records, kCuts);
    std::filesystem::remove_all(dir);
}

TEST(TestCutWriter, RotatedFilesEachParseStandalone)
{
    const auto dir = freshDir("cutwriter_rotation_integrity");
    constexpr uint32_t kCutEdges = 8;
    constexpr size_t kCuts = 130000;

    {
        CutWriter writer(dir, 0);
        const auto cut = makeCut(kCutEdges);
        for (size_t i = 0; i < kCuts; ++i)
        {
            writer.writeCut(cut);
        }
        writer.flush();
    }

    const auto files = test::listCutFiles(dir);
    ASSERT_GE(files.size(), 2U);

    for (const auto& path : files)
    {
        std::ifstream in(path, std::ios::binary);
        ASSERT_TRUE(in.is_open()) << path;

        const auto bytes = std::filesystem::file_size(path);
        std::vector<uint8_t> data(static_cast<size_t>(bytes));
        in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        ASSERT_EQ(static_cast<size_t>(in.gcount()), data.size()) << path;

        size_t offset = 16;
        size_t records = 0;
        while (offset < data.size())
        {
            ASSERT_GE(data.size() - offset, sizeof(uint32_t)) << "file ends inside a record header: " << path;

            uint32_t count = 0;
            std::memcpy(&count, data.data() + offset, sizeof(count));
            const size_t recordSize = sizeof(uint32_t) + static_cast<size_t>(count) * sizeof(uint32_t);

            ASSERT_LE(recordSize, data.size() - offset)
                << "file ends inside a record body (" << count << " edges): " << path;

            offset += recordSize;
            ++records;
        }

        EXPECT_EQ(offset, data.size()) << "file must end exactly after a whole record: " << path;
        EXPECT_GT(records, 0U) << path;
    }

    std::filesystem::remove_all(dir);
}

TEST(TestCutWriter, OpenFailureIsReportedInsteadOfSilentlyDroppingCuts)
{
    const auto dir = freshDir("cutwriter_readonly");
    std::filesystem::create_directories(dir);
    std::filesystem::permissions(dir, std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec,
                                 std::filesystem::perm_options::replace);

    {
        CutWriter writer(dir, 0);
        writer.writeCut(std::vector<EdgeIndex>{0, 1, 2});
        writer.flush();
        EXPECT_TRUE(writer.hasError()) << "a failed fopen must surface as an error, not a silent drop";
    }

    std::filesystem::permissions(dir, std::filesystem::perms::owner_all, std::filesystem::perm_options::replace);
    std::filesystem::remove_all(dir);
}
