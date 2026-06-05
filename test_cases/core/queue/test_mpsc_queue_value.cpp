#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <queue/mpsc_queue.h>

struct TaskEventValue
{
    size_t id = 0;
    std::string payload;

    TaskEventValue() = default;

    TaskEventValue(size_t i, std::string p)
        : id(i)
        , payload(std::move(p))
    {
    }

    TaskEventValue(std::nullptr_t)
        : id(0)
        , payload("")
    {
    }

    TaskEventValue& operator=(std::nullptr_t)
    {
        id = 0;
        payload.clear();
        return *this;
    }

    bool operator==(std::nullptr_t) const
    {
        return id == 0 && payload.empty();
    }
};

TEST(MPSCQueueValueTest, PopEmptyReturnsNullValue)
{
    MPSCQueue<TaskEventValue, 8> q;

    auto item = q.pop();

    ASSERT_TRUE(item == nullptr);
    ASSERT_EQ(q.size(), 0);
}

TEST(MPSCQueueValueTest, PushPopSingleItem)
{
    MPSCQueue<TaskEventValue, 8> q;

    q.push(TaskEventValue{1, "hello"});

    auto item = q.pop();

    ASSERT_EQ(item.id, 1);
    ASSERT_EQ(item.payload, "hello");
    ASSERT_TRUE(q.pop() == nullptr);
}

TEST(MPSCQueueValueTest, PreserveFIFOOrder)
{
    MPSCQueue<TaskEventValue, 8> q;

    q.push(TaskEventValue{1, "a"});
    q.push(TaskEventValue{2, "b"});
    q.push(TaskEventValue{3, "c"});

    ASSERT_EQ(q.pop().id, 1);
    ASSERT_EQ(q.pop().id, 2);
    ASSERT_EQ(q.pop().id, 3);
    ASSERT_TRUE(q.pop() == nullptr);
}

TEST(MPSCQueueValueTest, WrapAroundCorrectness)
{
    MPSCQueue<TaskEventValue, 4> q;

    for (size_t i = 1; i <= 1000; ++i)
    {
        q.push(TaskEventValue{i, "x"});

        auto item = q.pop();

        ASSERT_EQ(item.id, i);
        ASSERT_EQ(item.payload, "x");
    }

    ASSERT_TRUE(q.pop() == nullptr);
}

TEST(MPSCQueueValueTest, QueueFullThrows)
{
    MPSCQueue<TaskEventValue, 2> q;

    q.push(TaskEventValue{1, "a"});
    q.push(TaskEventValue{2, "b"});

    ASSERT_THROW(q.push(TaskEventValue{3, "c"}), std::runtime_error);
}

TEST(MPSCQueueValueTest, FullThenPopThenPushAgain)
{
    MPSCQueue<TaskEventValue, 2> q;

    q.push(TaskEventValue{1, "a"});
    q.push(TaskEventValue{2, "b"});

    ASSERT_EQ(q.pop().id, 1);

    q.push(TaskEventValue{3, "c"});

    ASSERT_EQ(q.pop().id, 2);
    ASSERT_EQ(q.pop().id, 3);
    ASSERT_TRUE(q.pop() == nullptr);
}

TEST(MPSCQueueValueTest, PushNullValueIsAllowed)
{
    MPSCQueue<TaskEventValue, 8> q;

    q.push(nullptr);

    auto item = q.pop();

    ASSERT_TRUE(item == nullptr);
}

TEST(MPSCQueueValueTest, SizeTwoFullThrows)
{
    MPSCQueue<TaskEventValue, 2> q;

    q.push(TaskEventValue{1, "a"});
    q.push(TaskEventValue{2, "b"});

    ASSERT_THROW(q.push(TaskEventValue{3, "c"}), std::runtime_error);

    ASSERT_EQ(q.pop().id, 1);
    ASSERT_EQ(q.pop().id, 2);
    ASSERT_TRUE(q.pop() == nullptr);
}

TEST(MPSCQueueValueTest, SizeTwoHeavyWrapAround)
{
    MPSCQueue<TaskEventValue, 2> q;

    for (size_t i = 1; i <= 10000; ++i)
    {
        q.push(TaskEventValue{i, "wrap"});

        auto item = q.pop();

        ASSERT_EQ(item.id, i);
        ASSERT_EQ(item.payload, "wrap");
    }

    ASSERT_TRUE(q.pop() == nullptr);
}

TEST(MPSCQueueValueTest, BurstPushBurstPopManyRounds)
{
    MPSCQueue<TaskEventValue, 16> q;

    size_t expected = 1;

    for (size_t round = 0; round < 1000; ++round)
    {
        for (size_t i = 0; i < 16; ++i)
        {
            q.push(TaskEventValue{expected + i, "burst"});
        }

        for (size_t i = 0; i < 16; ++i)
        {
            auto item = q.pop();

            ASSERT_EQ(item.id, expected++);
            ASSERT_EQ(item.payload, "burst");
        }

        ASSERT_TRUE(q.pop() == nullptr);
    }
}

TEST(MPSCQueueValueTest, ConsumerStartsBeforeProducer)
{
    MPSCQueue<TaskEventValue, 64> q;

    static constexpr size_t TOTAL = 10000;

    std::atomic<bool> start{false};
    std::atomic<size_t> empty_pops{0};
    std::vector<size_t> result;
    result.reserve(TOTAL);

    std::thread consumer([&]()
    {
        while (!start.load(std::memory_order_acquire))
        {
            auto item = q.pop();
            if (item == nullptr)
            {
                empty_pops.fetch_add(1, std::memory_order_relaxed);
            }
        }

        while (result.size() < TOTAL)
        {
            auto item = q.pop();

            if (item == nullptr)
            {
                std::this_thread::yield();
                continue;
            }

            result.push_back(item.id);
        }
    });

    std::thread producer([&]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        start.store(true, std::memory_order_release);

        for (size_t i = 1; i <= TOTAL; ++i)
        {
            while (true)
            {
                try
                {
                    q.push(TaskEventValue{i, "late"});
                    break;
                }
                catch (const std::runtime_error&)
                {
                    std::this_thread::yield();
                }
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_GT(empty_pops.load(), 0);
    ASSERT_EQ(result.size(), TOTAL);

    for (size_t i = 0; i < TOTAL; ++i)
    {
        ASSERT_EQ(result[i], i + 1);
    }
}

TEST(MPSCQueueValueTest, TinyQueueManyProducerStress)
{
    static constexpr size_t PRODUCERS = 8;
    static constexpr size_t ITEMS_PER_PRODUCER = 20000;
    static constexpr size_t TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    MPSCQueue<TaskEventValue, 4> q;

    std::atomic<bool> start{false};
    std::atomic<size_t> consumed{0};

    std::vector<std::thread> producers;

    for (size_t p = 0; p < PRODUCERS; ++p)
    {
        producers.emplace_back([&, p]()
        {
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (size_t i = 0; i < ITEMS_PER_PRODUCER; ++i)
            {
                size_t id = p * ITEMS_PER_PRODUCER + i + 1;

                while (true)
                {
                    try
                    {
                        q.push(TaskEventValue{id, "tiny"});
                        break;
                    }
                    catch (const std::runtime_error&)
                    {
                        std::this_thread::yield();
                    }
                }
            }
        });
    }

    std::unordered_set<size_t> seen;
    seen.reserve(TOTAL);

    std::thread consumer([&]()
    {
        while (consumed.load(std::memory_order_relaxed) < TOTAL)
        {
            auto item = q.pop();

            if (item == nullptr)
            {
                std::this_thread::yield();
                continue;
            }

            ASSERT_TRUE(seen.insert(item.id).second);
            ASSERT_EQ(item.payload, "tiny");

            consumed.fetch_add(1, std::memory_order_relaxed);
        }
    });

    start.store(true, std::memory_order_release);

    for (auto& t : producers)
    {
        t.join();
    }

    consumer.join();

    ASSERT_EQ(consumed.load(), TOTAL);
    ASSERT_EQ(seen.size(), TOTAL);
    ASSERT_TRUE(q.pop() == nullptr);
}

TEST(MPSCQueueValueTest, MultiProducerSingleConsumerStress)
{
    static constexpr size_t PRODUCERS = 4;
    static constexpr size_t ITEMS_PER_PRODUCER = 50000;
    static constexpr size_t TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    MPSCQueue<TaskEventValue, 4096> q;

    std::atomic<size_t> consumed{0};
    std::atomic<bool> start{false};

    std::vector<std::thread> producers;

    for (size_t p = 0; p < PRODUCERS; ++p)
    {
        producers.emplace_back([&, p]()
        {
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (size_t i = 0; i < ITEMS_PER_PRODUCER; ++i)
            {
                size_t id = p * ITEMS_PER_PRODUCER + i + 1;

                while (true)
                {
                    try
                    {
                        q.push(TaskEventValue{id, "payload"});
                        break;
                    }
                    catch (const std::runtime_error&)
                    {
                        std::this_thread::yield();
                    }
                }
            }
        });
    }

    std::unordered_set<size_t> seen;
    seen.reserve(TOTAL);

    std::thread consumer([&]()
    {
        while (consumed.load(std::memory_order_relaxed) < TOTAL)
        {
            auto item = q.pop();

            if (item == nullptr)
            {
                std::this_thread::yield();
                continue;
            }

            ASSERT_TRUE(seen.insert(item.id).second);
            ASSERT_EQ(item.payload, "payload");

            consumed.fetch_add(1, std::memory_order_relaxed);
        }
    });

    start.store(true, std::memory_order_release);

    for (auto& t : producers)
    {
        t.join();
    }

    consumer.join();

    ASSERT_EQ(consumed.load(), TOTAL);
    ASSERT_EQ(seen.size(), TOTAL);
    ASSERT_TRUE(q.pop() == nullptr);
}