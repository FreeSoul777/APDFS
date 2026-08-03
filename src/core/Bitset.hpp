#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

namespace apdfs
{

class DynamicBitset
{
  public:
    using Word = uint64_t;
    static constexpr size_t BITS_PER_WORD = 64;
    static constexpr size_t ALIGNMENT = 64;

    explicit DynamicBitset(size_t bitCount)
        : mBitSize(bitCount), mWordCount((bitCount + BITS_PER_WORD - 1) / BITS_PER_WORD)
    {
        if (mWordCount > 0)
        {
            mData = static_cast<Word*>(::operator new(mWordCount * sizeof(Word), std::align_val_t(ALIGNMENT)));
            resetAll();
        }
        else
        {
            mData = nullptr;
        }
    }

    ~DynamicBitset()
    {
        if (mData != nullptr)
        {
            ::operator delete(mData, std::align_val_t(ALIGNMENT));
        }
    }

    DynamicBitset(const DynamicBitset& other) : mBitSize(other.mBitSize), mWordCount(other.mWordCount)
    {
        if (mWordCount > 0)
        {
            mData = static_cast<Word*>(::operator new(mWordCount * sizeof(Word), std::align_val_t(ALIGNMENT)));
            std::memcpy(mData, other.mData, mWordCount * sizeof(Word));
        }
        else
        {
            mData = nullptr;
        }
    }

    DynamicBitset& operator=(const DynamicBitset& other)
    {
        if (this != &other)
        {
            DynamicBitset temp(other);
            swap(temp);
        }
        return *this;
    }

    DynamicBitset(DynamicBitset&& other) noexcept
        : mBitSize(other.mBitSize), mWordCount(other.mWordCount), mData(other.mData)
    {
        other.mData = nullptr;
        other.mBitSize = 0;
        other.mWordCount = 0;
    }

    DynamicBitset& operator=(DynamicBitset&& other) noexcept
    {
        if (this != &other)
        {
            if (mData != nullptr)
            {
                ::operator delete(mData, std::align_val_t(ALIGNMENT));
            }
            mBitSize = other.mBitSize;
            mWordCount = other.mWordCount;
            mData = other.mData;
            other.mData = nullptr;
            other.mBitSize = 0;
            other.mWordCount = 0;
        }
        return *this;
    }

    void set(size_t idx) noexcept
    {
        mData[idx / BITS_PER_WORD] |= (Word(1) << (idx % BITS_PER_WORD));
    }

    void reset(size_t idx) noexcept
    {
        mData[idx / BITS_PER_WORD] &= ~(Word(1) << (idx % BITS_PER_WORD));
    }

    bool test(size_t idx) const noexcept
    {
        return ((mData[idx / BITS_PER_WORD] >> (idx % BITS_PER_WORD)) & 1) != 0u;
    }

    void resetAll() noexcept
    {
        if (mData != nullptr && mWordCount > 0)
        {
            std::memset(mData, 0, mWordCount * sizeof(Word));
        }
    }

    void setAll() noexcept
    {
        if (mData != nullptr && mWordCount > 0)
        {
            std::memset(mData, 0xFF, mWordCount * sizeof(Word));
            clearPaddingBits();
        }
    }

    void copyFrom(const DynamicBitset& src) noexcept
    {
        if (mWordCount != src.mWordCount)
        {
            return;
        }
        if (mData != nullptr && src.mData != nullptr && mWordCount > 0)
        {
            std::memcpy(mData, src.mData, mWordCount * sizeof(Word));
        }
    }

    size_t size() const noexcept
    {
        return mBitSize;
    }

    size_t wordCount() const noexcept
    {
        return mWordCount;
    }

    const Word* data() const noexcept
    {
        return mData;
    }

    Word* data() noexcept
    {
        return mData;
    }

    template <typename Func>
    void forEachSet(Func&& f) const
    {
        for (size_t w = 0; w < mWordCount; ++w)
        {
            Word word = mData[w];
            if (word == 0)
            {
                continue;
            }
            size_t base = w * BITS_PER_WORD;
            while (word != 0)
            {
                int bit = __builtin_ctzll(word);
                f(base + static_cast<size_t>(bit));
                word &= word - 1;
            }
        }
    }

    void swap(DynamicBitset& other) noexcept
    {
        std::swap(mBitSize, other.mBitSize);
        std::swap(mWordCount, other.mWordCount);
        std::swap(mData, other.mData);
    }

  private:
    size_t mBitSize;
    size_t mWordCount;
    Word* mData;

    void clearPaddingBits() noexcept
    {
        size_t remaining = mBitSize % BITS_PER_WORD;
        if (remaining != 0 && mWordCount > 0)
        {
            mData[mWordCount - 1] &= (Word(1) << remaining) - 1;
        }
    }
};

} // namespace apdfs
