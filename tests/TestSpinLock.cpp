#include "core/SpinLock.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace apdfs;

TEST(TestSpinLock, LockUnlock_DoesNotDeadlock)
{
    SpinLock lock;
    lock.lock();
    lock.unlock();
    SUCCEED();
}

TEST(TestSpinLock, TryLock_AcquiresWhenFree)
{
    SpinLock lock;
    EXPECT_TRUE(lock.tryLock());
    lock.unlock();
}

TEST(TestSpinLock, TryLock_FailsWhenHeld)
{
    SpinLock lock;
    lock.lock();
    EXPECT_FALSE(lock.tryLock());
    lock.unlock();
}

TEST(TestSpinLock, LockUnlock_MultipleTimes)
{
    SpinLock lock;
    for (int i = 0; i < 1000; ++i)
    {
        lock.lock();
        lock.unlock();
    }
    SUCCEED();
}

TEST(TestSpinLock, MutualExclusion_TwoThreads)
{
    SpinLock lock;
    std::atomic<int> counter{0};
    const int iterations = 100000;

    auto worker = [&]() {
        for (int i = 0; i < iterations; ++i)
        {
            lock.lock();

            int v = counter.load(std::memory_order_relaxed);
            counter.store(v + 1, std::memory_order_relaxed);
            lock.unlock();
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();

    EXPECT_EQ(counter.load(), 2 * iterations);
}

TEST(TestSpinLock, TryLock_ManyThreads)
{
    SpinLock lock;
    std::atomic<int> successes{0};
    std::atomic<bool> running{true};

    auto worker = [&]() {
        while (running.load(std::memory_order_relaxed))
        {
            if (lock.tryLock())
            {
                successes.fetch_add(1, std::memory_order_relaxed);
                lock.unlock();
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(4);
    for (int i = 0; i < 4; ++i)
    {
        threads.emplace_back(worker);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running.store(false, std::memory_order_relaxed);

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_GT(successes.load(), 0);
}

TEST(TestSpinLock, Size_IsCacheLine)
{
    EXPECT_EQ(sizeof(SpinLock), 64U);
}

TEST(TestSpinLock, Alignment_IsCacheLine)
{
    alignas(SpinLock) char buf[sizeof(SpinLock)];
    auto ptr = reinterpret_cast<uintptr_t>(&buf);
    EXPECT_EQ(ptr % 64, 0U);
}

TEST(TestSpinLock, NoFalseSharing_TwoLocks)
{
    alignas(64) SpinLock lock1;
    alignas(64) SpinLock lock2;

    std::atomic<bool> t1Done{false};
    std::atomic<bool> t2Done{false};

    auto worker1 = [&]() {
        for (int i = 0; i < 100000; ++i)
        {
            lock1.lock();
            lock1.unlock();
        }
        t1Done.store(true, std::memory_order_release);
    };

    auto worker2 = [&]() {
        for (int i = 0; i < 100000; ++i)
        {
            lock2.lock();
            lock2.unlock();
        }
        t2Done.store(true, std::memory_order_release);
    };

    std::thread t1(worker1);
    std::thread t2(worker2);

    t1.join();
    t2.join();

    EXPECT_TRUE(t1Done.load());
    EXPECT_TRUE(t2Done.load());
}
