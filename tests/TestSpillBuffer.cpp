#include "cut/SpillBuffer.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <set>
#include <thread>
#include <vector>

using namespace apdfs;

namespace
{

Frame makeFrame(EdgeIndex marker)
{
    return Frame{Hash{marker, marker}, std::vector<EdgeIndex>{marker}, 0};
}

} // namespace

TEST(TestSpillBuffer, PushThenPopPreservesAllFrames)
{
    SpillBuffer spill;

    std::vector<Frame> frames;
    for (EdgeIndex i = 0; i < 10; ++i)
    {
        frames.push_back(makeFrame(i));
    }

    spill.push(frames);
    EXPECT_TRUE(frames.empty()) << "push must move the frames out of the caller's buffer";
    EXPECT_EQ(spill.size(), 10U);

    std::vector<Frame> out;
    ASSERT_TRUE(spill.pop(out, 4));
    EXPECT_EQ(out.size(), 4U);
    EXPECT_EQ(spill.size(), 6U);

    ASSERT_TRUE(spill.pop(out, 100));
    EXPECT_EQ(out.size(), 6U);
    EXPECT_EQ(spill.size(), 0U);

    EXPECT_FALSE(spill.pop(out, 100)) << "popping an empty buffer must return false";
    EXPECT_TRUE(out.empty());
}

TEST(TestSpillBuffer, PopReturnsAtMostMaxCount)
{
    SpillBuffer spill;
    std::vector<Frame> frames;
    for (EdgeIndex i = 0; i < 5; ++i)
    {
        frames.push_back(makeFrame(i));
    }
    spill.push(frames);

    std::vector<Frame> out;
    ASSERT_TRUE(spill.pop(out, 2));
    EXPECT_EQ(out.size(), 2U);
    EXPECT_EQ(spill.size(), 3U);
}

TEST(TestSpillBuffer, ConcurrentProducersAndConsumersMoveEveryFrameExactlyOnce)
{
    SpillBuffer spill;
    constexpr size_t kProducers = 4;
    constexpr size_t kFramesPerProducer = 500;
    constexpr size_t kConsumers = 4;

    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};
    std::atomic<bool> done{false};

    std::vector<std::thread> producers;
    for (size_t p = 0; p < kProducers; ++p)
    {
        producers.emplace_back([&spill, p, &produced] {
            std::vector<Frame> batch;
            for (size_t i = 0; i < kFramesPerProducer; ++i)
            {
                batch.push_back(makeFrame(static_cast<EdgeIndex>(p * kFramesPerProducer + i)));
                if (batch.size() == 16)
                {
                    produced.fetch_add(batch.size(), std::memory_order_relaxed);
                    spill.push(batch);
                }
            }
            produced.fetch_add(batch.size(), std::memory_order_relaxed);
            spill.push(batch);
        });
    }

    std::vector<std::thread> consumers;
    for (size_t c = 0; c < kConsumers; ++c)
    {
        consumers.emplace_back([&spill, &consumed, &done] {
            std::vector<Frame> out;
            while (!done.load(std::memory_order_acquire) || spill.size() != 0)
            {
                if (spill.pop(out, 32))
                {
                    consumed.fetch_add(out.size(), std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& t : producers)
    {
        t.join();
    }
    done.store(true, std::memory_order_release);
    for (auto& t : consumers)
    {
        t.join();
    }

    EXPECT_EQ(produced.load(), kProducers * kFramesPerProducer);
    EXPECT_EQ(consumed.load(), produced.load()) << "a frame was lost or duplicated in the spill buffer";
    EXPECT_EQ(spill.size(), 0U);
}
