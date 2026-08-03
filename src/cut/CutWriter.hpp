#pragma once

#include "core/Types.hpp"

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
          mFileIndex(other.mFileIndex),
          mBytesWritten(other.mBytesWritten),
          mFilesRotated(other.mFilesRotated)
    {
    }

    CutWriter& operator=(CutWriter&&) = delete;

    void writeCut(const std::vector<EdgeIndex>& cut);
    void flush();

    uint64_t totalBytesWritten() const noexcept
    {
        return mBytesWritten;
    }

    uint32_t filesRotated() const noexcept
    {
        return mFilesRotated;
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
    uint64_t mBytesWritten;
    uint32_t mFilesRotated;

    void openNewFile();
    void flushBuffer();
};

} // namespace apdfs
