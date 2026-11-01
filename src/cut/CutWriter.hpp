#pragma once

#include "core/Types.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace apdfs
{

class CutWriter
{
  public:
    static constexpr size_t MAX_FILE_SIZE = 1024 * 1024 * 4;
    static constexpr size_t BUFFER_CAPACITY = 1024 * 1024;

    CutWriter(const std::filesystem::path& outputDir, uint32_t threadId);
    ~CutWriter() noexcept;

    CutWriter(const CutWriter&) = delete;
    CutWriter& operator=(const CutWriter&) = delete;

    CutWriter(CutWriter&& other) noexcept
        : mOutputDir(std::move(other.mOutputDir)),
          mThreadId(other.mThreadId),
          mFile(std::exchange(other.mFile, nullptr)),
          mBuffer(std::move(other.mBuffer)),
          mBufferUsed(other.mBufferUsed),
          mCurrentFileSize(other.mCurrentFileSize),
          mFileIndex(other.mFileIndex)
    {
        mBytesWritten.store(other.mBytesWritten.load(std::memory_order_relaxed), std::memory_order_relaxed);
        mFilesRotated.store(other.mFilesRotated.load(std::memory_order_relaxed), std::memory_order_relaxed);
        mError.store(other.mError.load(std::memory_order_relaxed), std::memory_order_relaxed);
        other.mBytesWritten.store(0, std::memory_order_relaxed);
        other.mFilesRotated.store(0, std::memory_order_relaxed);
        other.mBufferUsed = 0;
    }

    CutWriter& operator=(CutWriter&&) = delete;

    void writeCut(const std::vector<EdgeIndex>& cut);
    void flush();

    uint64_t totalBytesWritten() const noexcept
    {
        return mBytesWritten.load(std::memory_order_relaxed);
    }

    uint32_t filesRotated() const noexcept
    {
        return mFilesRotated.load(std::memory_order_relaxed);
    }

    bool hasError() const noexcept
    {
        return mError.load(std::memory_order_relaxed);
    }

  private:
    struct FileDeleter
    {
        void operator()(FILE* f) const noexcept
        {
            if (f != nullptr)
            {
                std::fclose(f);
            }
        }
    };

    std::filesystem::path mOutputDir;
    uint32_t mThreadId;
    std::unique_ptr<FILE, FileDeleter> mFile;
    std::vector<uint8_t> mBuffer;
    size_t mBufferUsed;
    uint64_t mCurrentFileSize;
    uint32_t mFileIndex;
    std::atomic<uint64_t> mBytesWritten{0};
    std::atomic<uint32_t> mFilesRotated{0};
    std::atomic<bool> mError{false};

    void openNewFile();
    void flushBuffer();
    void writeDirectRecord(const std::vector<EdgeIndex>& cut, uint32_t cutSize, size_t recordSize);
};

} // namespace apdfs
