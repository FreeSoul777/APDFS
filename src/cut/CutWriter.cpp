#include "cut/CutWriter.hpp"

#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace apdfs
{

static constexpr uint32_t MAGIC = 0x4544484D;
static constexpr uint32_t VERSION = 1;

static std::string generateFilename(uint32_t threadId, uint32_t index)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&time, &tm);

    std::ostringstream oss;
    oss << "cuts_" << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900) << std::setw(2) << (tm.tm_mon + 1)
        << std::setw(2) << tm.tm_mday << '_' << std::setw(2) << tm.tm_hour << std::setw(2) << tm.tm_min << std::setw(2)
        << tm.tm_sec << '_' << "t" << threadId << '_' << std::setw(4) << index << ".bin";
    return oss.str();
}

CutWriter::CutWriter(const std::filesystem::path& outputDir, uint32_t threadId)
    : mOutputDir(outputDir),
      mThreadId(threadId),
      mFile(nullptr),
      mBufferUsed(0),
      mCurrentFileSize(0),
      mFileIndex(0),
      mBytesWritten(0),
      mFilesRotated(0)
{
    std::filesystem::create_directories(mOutputDir);
    mBuffer.resize(BUFFER_CAPACITY);
    openNewFile();
}

CutWriter::~CutWriter() noexcept
{
    flush();
}

void CutWriter::writeCut(const std::vector<EdgeIndex>& cut)
{
    auto cutSize = static_cast<uint32_t>(cut.size());
    size_t recordSize = sizeof(uint32_t) + cutSize * sizeof(EdgeIndex);

    if (mBufferUsed + recordSize > BUFFER_CAPACITY)
    {
        flushBuffer();
    }

    if (recordSize > BUFFER_CAPACITY)
    {
        if (mCurrentFileSize + recordSize > MAX_FILE_SIZE)
        {
            openNewFile();
        }
        std::fwrite(&cutSize, sizeof(uint32_t), 1, mFile.get());
        std::fwrite(cut.data(), sizeof(EdgeIndex), cutSize, mFile.get());
        mCurrentFileSize += recordSize;
        mBytesWritten += recordSize;
        return;
    }

    std::memcpy(mBuffer.data() + mBufferUsed, &cutSize, sizeof(uint32_t));
    mBufferUsed += sizeof(uint32_t);
    std::memcpy(mBuffer.data() + mBufferUsed, cut.data(), cutSize * sizeof(EdgeIndex));
    mBufferUsed += cutSize * sizeof(EdgeIndex);
}

void CutWriter::flush()
{
    flushBuffer();
}

void CutWriter::openNewFile()
{
    if (mFile != nullptr)
    {
        mFile.reset();
        mFileIndex++;
        mFilesRotated++;
    }

    std::string filename = generateFilename(mThreadId, mFileIndex);
    std::filesystem::path filepath = mOutputDir / filename;
    FILE* f = std::fopen(filepath.c_str(), "wb");
    if (f == nullptr)
    {
        std::fprintf(stderr, "Warning: Failed to open output file: %s\n", filepath.c_str());
        mFile.reset(nullptr);
        return;
    }
    mFile.reset(f);

    uint32_t header[4] = {MAGIC, VERSION, 0, 0};
    std::fwrite(header, sizeof(uint32_t), 4, mFile.get());
    mCurrentFileSize = sizeof(header);
    mBytesWritten += sizeof(header);
}

void CutWriter::flushBuffer()
{
    if (mBufferUsed == 0)
    {
        return;
    }

    if (mFile == nullptr)
    {
        std::string filename = generateFilename(mThreadId, mFileIndex);
        std::filesystem::path filepath = mOutputDir / filename;
        FILE* f = std::fopen(filepath.c_str(), "wb");
        if (f == nullptr)
        {
            std::fprintf(stderr, "Warning: Failed to open output file: %s\n", filepath.c_str());
            return;
        }
        mFile.reset(f);

        uint32_t header[4] = {MAGIC, VERSION, 0, 0};
        std::fwrite(header, sizeof(uint32_t), 4, mFile.get());
        mCurrentFileSize = sizeof(header);
        mBytesWritten += sizeof(header);
    }

    size_t toWrite = mBufferUsed;
    size_t written = 0;

    while (written < toWrite)
    {
        if (mCurrentFileSize >= MAX_FILE_SIZE)
        {
            mFile.reset();
            mFileIndex++;
            mFilesRotated++;

            std::string filename = generateFilename(mThreadId, mFileIndex);
            std::filesystem::path filepath = mOutputDir / filename;
            FILE* f = std::fopen(filepath.c_str(), "wb");
            if (f == nullptr)
            {
                std::fprintf(stderr, "Warning: Failed to open output file: %s\n", filepath.c_str());
                return;
            }
            mFile.reset(f);

            uint32_t header[4] = {MAGIC, VERSION, 0, 0};
            std::fwrite(header, sizeof(uint32_t), 4, mFile.get());
            mCurrentFileSize = sizeof(header);
            mBytesWritten += sizeof(header);
        }

        size_t spaceLeft = MAX_FILE_SIZE - mCurrentFileSize;
        size_t chunk = std::min(toWrite - written, spaceLeft);

        std::fwrite(mBuffer.data() + written, 1, chunk, mFile.get());
        written += chunk;
        mCurrentFileSize += chunk;
        mBytesWritten += chunk;
    }

    mBufferUsed = 0;
}

} // namespace apdfs
