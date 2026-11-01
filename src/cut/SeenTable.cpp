#include "cut/SeenTable.hpp"

namespace apdfs
{

bool SeenTable::tryMarkProcessing(Hash key) noexcept
{
    if (!segment(key).tryInsert(key))
    {
        return false;
    }
    mEntryCount.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool SeenTable::contains(Hash key) noexcept
{
    return segment(key).contains(key);
}

size_t SeenTable::size() const noexcept
{
    return mEntryCount.load(std::memory_order_relaxed);
}

bool SeenTable::Segment::tryInsert(Hash key) noexcept
{
    std::lock_guard<SpinLock> guard(mLock.lock);

    if (static_cast<double>(mData.mOccupied) / static_cast<double>(mData.mKeys.size()) > MAX_LOAD_FACTOR)
    {
        rehash();
    }

    const size_t capacity = mData.mKeys.size();
    size_t index = hashToIndex(key);

    for (size_t probe = 0; probe < capacity; ++probe)
    {
        if (!mData.isOccupied(index))
        {
            mData.mKeys[index] = key;
            mData.setOccupied(index);
            ++mData.mOccupied;
            return true;
        }

        if (mData.mKeys[index] == key)
        {
            return false;
        }

        index = (index + 1) & mData.mCapacityMask;
    }

    return false;
}

bool SeenTable::Segment::contains(Hash key) noexcept
{
    std::lock_guard<SpinLock> guard(mLock.lock);

    const size_t capacity = mData.mKeys.size();
    size_t index = hashToIndex(key);

    for (size_t probe = 0; probe < capacity; ++probe)
    {
        if (!mData.isOccupied(index))
        {
            return false;
        }

        if (mData.mKeys[index] == key)
        {
            return true;
        }

        index = (index + 1) & mData.mCapacityMask;
    }

    return false;
}

void SeenTable::Segment::rehash()
{
    std::vector<Hash> oldKeys = std::move(mData.mKeys);
    std::vector<uint64_t> oldOccupied = std::move(mData.mOccupiedBits);
    const size_t oldCapacity = oldKeys.size();

    mData.allocate(oldCapacity * 2);

    for (size_t index = 0; index < oldCapacity; ++index)
    {
        if (((oldOccupied[index >> 6] >> (index & 63)) & 1U) != 0U)
        {
            forceInsert(oldKeys[index]);
        }
    }
}

void SeenTable::Segment::forceInsert(Hash key) noexcept
{
    const size_t capacity = mData.mKeys.size();
    size_t index = hashToIndex(key);

    for (size_t probe = 0; probe < capacity; ++probe)
    {
        if (!mData.isOccupied(index))
        {
            mData.mKeys[index] = key;
            mData.setOccupied(index);
            ++mData.mOccupied;
            return;
        }

        index = (index + 1) & mData.mCapacityMask;
    }
}

} // namespace apdfs
