#include "cut/CutWriter.hpp"

#include "core/CutFormat.hpp"

#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace apdfs
{

namespace
{

std::string generateFilename(uint32_t threadId, uint32_t index)
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

size_t recordSizeAt(const uint8_t* bytes)
{
    uint32_t count = 0;
    std::memcpy(&count, bytes, sizeof(count));
    return sizeof(uint32_t) + static_cast<size_t>(count) * sizeof(EdgeIndex);
}

} // namespace

CutWriter::CutWriter(const std::filesystem::path& outputDir, uint32_t threadId)
    : mOutputDir(outputDir), mThreadId(threadId), mFile(nullptr), mBufferUsed(0), mCurrentFileSize(0), mFileIndex(0)
{
    std::filesystem::create_directories(mOutputDir);
    mBuffer.resize(BUFFER_CAPACITY);
    openNewFile();
}

CutWriter::~CutWriter() noexcept
{
    flushBuffer();

    if (mFile != nullptr)
    {
        std::fflush(mFile.get());
    }
}

void CutWriter::writeCut(const std::vector<EdgeIndex>& cut)
{
    if (mError.load(std::memory_order_relaxed))
    {
        return;
    }

    const auto cutSize = static_cast<uint32_t>(cut.size());
    const size_t recordSize = sizeof(uint32_t) + static_cast<size_t>(cutSize) * sizeof(EdgeIndex);

    if (recordSize > BUFFER_CAPACITY)
    {
        writeDirectRecord(cut, cutSize, recordSize);
        return;
    }

    if (mBufferUsed + recordSize > BUFFER_CAPACITY)
    {
        flushBuffer();
        if (mError.load(std::memory_order_relaxed))
        {
            return;
        }
    }

    std::memcpy(mBuffer.data() + mBufferUsed, &cutSize, sizeof(cutSize));
    mBufferUsed += sizeof(cutSize);

    if (cutSize != 0)
    {
        std::memcpy(mBuffer.data() + mBufferUsed, cut.data(), static_cast<size_t>(cutSize) * sizeof(EdgeIndex));
        mBufferUsed += static_cast<size_t>(cutSize) * sizeof(EdgeIndex);
    }
}

void CutWriter::flush()
{
    flushBuffer();

    if (mFile != nullptr)
    {
        std::fflush(mFile.get());
    }
}

void CutWriter::openNewFile()
{
    if (mFile != nullptr)
    {
        mFile.reset();
        mFileIndex++;
        mFilesRotated.fetch_add(1, std::memory_order_relaxed);
    }

    const std::string filename = generateFilename(mThreadId, mFileIndex);
    const std::filesystem::path filepath = mOutputDir / filename;
    FILE* file = std::fopen(filepath.c_str(), "wb");
    if (file == nullptr)
    {
        std::fprintf(stderr, "Warning: Failed to open output file: %s\n", filepath.c_str());
        mFile.reset(nullptr);
        mError.store(true, std::memory_order_relaxed);
        return;
    }
    mFile.reset(file);

    uint32_t header[CUT_FILE_HEADER_WORDS] = {CUT_FILE_MAGIC, CUT_FILE_VERSION, 0, 0};
    if (std::fwrite(header, sizeof(uint32_t), CUT_FILE_HEADER_WORDS, mFile.get()) != CUT_FILE_HEADER_WORDS)
    {
        mError.store(true, std::memory_order_relaxed);
        return;
    }

    mCurrentFileSize = sizeof(header);
    mBytesWritten.fetch_add(sizeof(header), std::memory_order_relaxed);
}

void CutWriter::writeDirectRecord(const std::vector<EdgeIndex>& cut, uint32_t cutSize, size_t recordSize)
{
    if (mFile == nullptr)
    {
        openNewFile();
        if (mFile == nullptr)
        {
            return;
        }
    }

    if (mCurrentFileSize + recordSize > MAX_FILE_SIZE)
    {
        openNewFile();
        if (mFile == nullptr)
        {
            return;
        }
    }

    const size_t wroteCount = std::fwrite(&cutSize, sizeof(cutSize), 1, mFile.get());
    const size_t wroteEdges = (cutSize == 0) ? 0U : std::fwrite(cut.data(), sizeof(EdgeIndex), cutSize, mFile.get());
    if (wroteCount != 1 || wroteEdges != cutSize)
    {
        mError.store(true, std::memory_order_relaxed);
        return;
    }

    mCurrentFileSize += recordSize;
    mBytesWritten.fetch_add(recordSize, std::memory_order_relaxed);
}

void CutWriter::flushBuffer()
{
    if (mBufferUsed == 0 || mError.load(std::memory_order_relaxed))
    {
        return;
    }

    size_t offset = 0;
    while (offset < mBufferUsed)
    {
        const size_t recordSize = recordSizeAt(mBuffer.data() + offset);
        if (recordSize > mBufferUsed - offset)
        {
            mError.store(true, std::memory_order_relaxed);
            mBufferUsed = 0;
            return;
        }

        if (mFile == nullptr)
        {
            openNewFile();
            if (mFile == nullptr)
            {
                return;
            }
        }

        if (mCurrentFileSize + recordSize > MAX_FILE_SIZE)
        {
            openNewFile();
            if (mFile == nullptr)
            {
                return;
            }
        }

        if (std::fwrite(mBuffer.data() + offset, 1, recordSize, mFile.get()) != recordSize)
        {
            mError.store(true, std::memory_order_relaxed);
            return;
        }

        mCurrentFileSize += recordSize;
        mBytesWritten.fetch_add(recordSize, std::memory_order_relaxed);
        offset += recordSize;
    }

    mBufferUsed = 0;
}

} // namespace apdfs
