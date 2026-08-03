#include "cut/SeenTable.hpp"

namespace apdfs
{

bool SeenTable::tryMarkProcessing(Hash key) noexcept
{
    return segment(key).tryInsert(key);
}

bool SeenTable::contains(Hash key) noexcept
{
    return segment(key).contains(key);
}

void SeenTable::markDone(Hash key) noexcept
{
    segment(key).markDone(key);
}

size_t SeenTable::size() const noexcept
{
    size_t total = 0;
    for (size_t i = 0; i < mNumSegments; ++i)
    {
        total += mSegments[i].mData.mOccupied;
    }
    return total;
}

bool SeenTable::Segment::tryInsert(Hash key) noexcept
{
    std::lock_guard<SpinLock> guard(mLock.lock);

    if (static_cast<double>(mData.mOccupied) / static_cast<double>(mData.mSlots.size()) > MAX_LOAD_FACTOR)
    {
        rehash();
    }

    Slot newSlot{key, State::PROCESSING, 0, true};
    size_t idx = hashToIndex(key);

    for (uint8_t dist = 0; dist < 128; ++dist)
    {
        Slot& current = mData.mSlots[idx];

        if (!current.occupied)
        {
            current = newSlot;
            current.distance = dist;
            mData.mOccupied++;
            return true;
        }

        if (current.key == key)
        {
            return false;
        }

        if (dist > current.distance)
        {
            std::swap(newSlot, current);
            dist = current.distance;
        }

        idx = (idx + 1) & mData.mCapacityMask;
        newSlot.distance++;
    }

    return false;
}

bool SeenTable::Segment::contains(Hash key) noexcept
{
    std::lock_guard<SpinLock> guard(mLock.lock);

    size_t idx = hashToIndex(key);
    for (uint8_t dist = 0; dist < 128; ++dist)
    {
        const Slot& slot = mData.mSlots[idx];

        if (!slot.occupied)
        {
            return false;
        }

        if (slot.key == key)
        {
            return true;
        }

        if (dist > slot.distance)
        {
            return false;
        }

        idx = (idx + 1) & mData.mCapacityMask;
    }

    return false;
}

void SeenTable::Segment::markDone(Hash key) noexcept
{
    std::lock_guard<SpinLock> guard(mLock.lock);

    size_t idx = hashToIndex(key);
    for (uint8_t dist = 0; dist < 128; ++dist)
    {
        Slot& slot = mData.mSlots[idx];

        if (!slot.occupied)
        {
            return;
        }

        if (slot.key == key)
        {
            slot.state = State::DONE;
            return;
        }

        if (dist > slot.distance)
        {
            return;
        }

        idx = (idx + 1) & mData.mCapacityMask;
    }
}

void SeenTable::Segment::rehash() noexcept
{
    std::vector<Slot> oldSlots = std::move(mData.mSlots);
    size_t newCapacity = oldSlots.size() * 2;
    mData.mSlots.resize(newCapacity);
    mData.mCapacityMask = newCapacity - 1;
    mData.mOccupied = 0;
    mData.clearSlots();

    for (const auto& slot : oldSlots)
    {
        if (slot.occupied)
        {
            forceInsert(slot.key, slot.state);
        }
    }
}

void SeenTable::Segment::forceInsert(Hash key, State state) noexcept
{
    Slot newSlot{key, state, 0, true};
    size_t idx = hashToIndex(key);

    for (uint8_t dist = 0; dist < 128; ++dist)
    {
        Slot& current = mData.mSlots[idx];

        if (!current.occupied)
        {
            current = newSlot;
            current.distance = dist;
            mData.mOccupied++;
            return;
        }

        if (dist > current.distance)
        {
            std::swap(newSlot, current);
            dist = current.distance;
        }

        idx = (idx + 1) & mData.mCapacityMask;
        newSlot.distance++;
    }
}

} // namespace apdfs
