#include "core/Shutdown.hpp"

#include <csignal>

namespace apdfs
{

std::atomic<bool> gShutdownRequested{false};

static void signalHandler(int /*unused*/)
{
    gShutdownRequested.store(true, std::memory_order_release);
}

void installSignalHandlers()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

} // namespace apdfs
