#pragma once

#include <atomic>

namespace apdfs
{

extern std::atomic<bool> gShutdownRequested;

void installSignalHandlers();

inline bool isShutdownRequested() noexcept
{
    return gShutdownRequested.load(std::memory_order_acquire);
}

} // namespace apdfs
