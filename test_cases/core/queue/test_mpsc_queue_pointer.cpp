#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <queue/mpsc_queue.h>

struct TaskEventPointer
{
    size_t id = 0;
    std::string payload;
};

TEST(MPSCQueuePointerTest, PopEmptyReturnsNullptr)
{
    MPSCQueue<TaskEventPointer*, 8> q;

    auto item = q.pop();

    ASSERT_EQ(item, nullptr);
    ASSERT_EQ(q.size(), 0);
}

TEST(MPSCQueuePointerTest, PushPopSingleItem)
{
    MPSCQueue<TaskEventPointer*, 8> q;

    auto obj = std::make_unique<TaskEventPointer>();
    obj->id = 1;
    obj->payload = "hello";

    q.push(obj.get());

    auto item = q.pop();

    ASSERT_NE(item, nullptr);
    ASSERT_EQ(item->id, 1);
    ASSERT_EQ(item->payload, "hello");
    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, PreserveFIFOOrder)
{
    MPSCQueue<TaskEventPointer*, 8> q;

    TaskEventPointer a{1, "a"};
    TaskEventPointer b{2, "b"};
    TaskEventPointer c{3, "c"};

    q.push(&a);
    q.push(&b);
    q.push(&c);

    ASSERT_EQ(q.pop()->id, 1);
    ASSERT_EQ(q.pop()->id, 2);
    ASSERT_EQ(q.pop()->id, 3);
    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, WrapAroundCorrectness)
{
    MPSCQueue<TaskEventPointer*, 4> q;

    std::vector<TaskEventPointer> storage(1001);

    for (size_t i = 1; i <= 1000; ++i)
    {
        storage[i].id = i;
        storage[i].payload = "x";

        q.push(&storage[i]);

        auto item = q.pop();

        ASSERT_NE(item, nullptr);
        ASSERT_EQ(item->id, i);
        ASSERT_EQ(item->payload, "x");
    }

    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, QueueFullThrows)
{
    MPSCQueue<TaskEventPointer*, 2> q;

    TaskEventPointer a{1, "a"};
    TaskEventPointer b{2, "b"};
    TaskEventPointer c{3, "c"};

    q.push(&a);
    q.push(&b);

    ASSERT_THROW(q.push(&c), std::runtime_error);
}

TEST(MPSCQueuePointerTest, FullThenPopThenPushAgain)
{
    MPSCQueue<TaskEventPointer*, 2> q;

    TaskEventPointer a{1, "a"};
    TaskEventPointer b{2, "b"};
    TaskEventPointer c{3, "c"};

    q.push(&a);
    q.push(&b);

    ASSERT_EQ(q.pop()->id, 1);

    q.push(&c);

    ASSERT_EQ(q.pop()->id, 2);
    ASSERT_EQ(q.pop()->id, 3);
    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, PushNullptrThrows)
{
    MPSCQueue<TaskEventPointer*, 8> q;

    ASSERT_THROW(q.push(nullptr), std::runtime_error);
}

TEST(MPSCQueuePointerTest, SizeTwoFullThrows)
{
    MPSCQueue<TaskEventPointer*, 2> q;

    TaskEventPointer a{1, "a"};
    TaskEventPointer b{2, "b"};
    TaskEventPointer c{3, "c"};

    q.push(&a);
    q.push(&b);

    ASSERT_THROW(q.push(&c), std::runtime_error);

    ASSERT_EQ(q.pop()->id, 1);
    ASSERT_EQ(q.pop()->id, 2);
    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, SizeTwoHeavyWrapAround)
{
    MPSCQueue<TaskEventPointer*, 2> q;

    std::vector<TaskEventPointer> storage(10001);

    for (size_t i = 1; i <= 10000; ++i)
    {
        storage[i].id = i;
        storage[i].payload = "wrap";

        q.push(&storage[i]);

        auto item = q.pop();

        ASSERT_NE(item, nullptr);
        ASSERT_EQ(item->id, i);
        ASSERT_EQ(item->payload, "wrap");
    }

    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, BurstPushBurstPopManyRounds)
{
    MPSCQueue<TaskEventPointer*, 16> q;

    std::vector<TaskEventPointer> storage(16000 + 1);

    size_t expected = 1;

    for (size_t round = 0; round < 1000; ++round)
    {
        for (size_t i = 0; i < 16; ++i)
        {
            size_t id = expected + i;
            storage[id].id = id;
            storage[id].payload = "burst";

            q.push(&storage[id]);
        }

        for (size_t i = 0; i < 16; ++i)
        {
            auto item = q.pop();

            ASSERT_NE(item, nullptr);
            ASSERT_EQ(item->id, expected++);
            ASSERT_EQ(item->payload, "burst");
        }

        ASSERT_EQ(q.pop(), nullptr);
    }
}

TEST(MPSCQueuePointerTest, ConsumerStartsBeforeProducer)
{
    MPSCQueue<TaskEventPointer*, 64> q;

    static constexpr size_t TOTAL = 10000;

    std::vector<TaskEventPointer> storage(TOTAL + 1);

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

            result.push_back(item->id);
        }
    });

    std::thread producer([&]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        start.store(true, std::memory_order_release);

        for (size_t i = 1; i <= TOTAL; ++i)
        {
            storage[i].id = i;
            storage[i].payload = "late";

            while (true)
            {
                try
                {
                    q.push(&storage[i]);
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

TEST(MPSCQueuePointerTest, TinyQueueManyProducerStress)
{
    static constexpr size_t PRODUCERS = 8;
    static constexpr size_t ITEMS_PER_PRODUCER = 20000;
    static constexpr size_t TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    MPSCQueue<TaskEventPointer*, 4> q;

    std::vector<std::unique_ptr<TaskEventPointer>> storage;
    storage.reserve(TOTAL);

    for (size_t i = 0; i < TOTAL; ++i)
    {
        auto obj = std::make_unique<TaskEventPointer>();
        obj->id = i + 1;
        obj->payload = "tiny";
        storage.emplace_back(std::move(obj));
    }

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
                size_t index = p * ITEMS_PER_PRODUCER + i;

                while (true)
                {
                    try
                    {
                        q.push(storage[index].get());
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

            ASSERT_TRUE(seen.insert(item->id).second);
            ASSERT_EQ(item->payload, "tiny");

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
    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, MultiProducerSingleConsumerStress)
{
    static constexpr size_t PRODUCERS = 4;
    static constexpr size_t ITEMS_PER_PRODUCER = 50000;
    static constexpr size_t TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    MPSCQueue<TaskEventPointer*, 4096> q;

    std::vector<std::unique_ptr<TaskEventPointer>> storage;
    storage.reserve(TOTAL);

    for (size_t i = 0; i < TOTAL; ++i)
    {
        auto obj = std::make_unique<TaskEventPointer>();
        obj->id = i + 1;
        obj->payload = "payload";
        storage.emplace_back(std::move(obj));
    }

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
                size_t index = p * ITEMS_PER_PRODUCER + i;

                while (true)
                {
                    try
                    {
                        q.push(storage[index].get());
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

            ASSERT_TRUE(seen.insert(item->id).second);
            ASSERT_EQ(item->payload, "payload");

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
    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, LongStressWithRandomYield)
{
    static constexpr size_t PRODUCERS = 4;
    static constexpr size_t ITEMS_PER_PRODUCER = 1250000;
    static constexpr size_t TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    MPSCQueue<TaskEventPointer*, 8192> q;

    std::vector<std::unique_ptr<TaskEventPointer>> storage;
    storage.reserve(TOTAL);

    for (size_t i = 0; i < TOTAL; ++i)
    {
        auto obj = std::make_unique<TaskEventPointer>();
        obj->id = i + 1;
        obj->payload = "long";
        storage.emplace_back(std::move(obj));
    }

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
                size_t index = p * ITEMS_PER_PRODUCER + i;

                if ((i & 0x3FF) == 0)
                {
                    std::this_thread::yield();
                }

                while (true)
                {
                    try
                    {
                        q.push(storage[index].get());
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

    std::vector<std::atomic<uint8_t>> seen(TOTAL + 1);

    for (auto& v : seen)
    {
        v.store(0, std::memory_order_relaxed);
    }

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

            ASSERT_GE(item->id, 1);
            ASSERT_LE(item->id, TOTAL);
            ASSERT_EQ(item->payload, "long");

            uint8_t expected = 0;

            ASSERT_TRUE(
                seen[item->id].compare_exchange_strong(
                    expected,
                    1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed
                )
            );

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

    for (size_t id = 1; id <= TOTAL; ++id)
    {
        ASSERT_EQ(seen[id].load(std::memory_order_relaxed), 1);
    }

    ASSERT_EQ(q.pop(), nullptr);
}

TEST(MPSCQueuePointerTest, ThreadSanitizerStress)
{
    static constexpr size_t PRODUCERS = 4;
    static constexpr size_t ITEMS_PER_PRODUCER = 100000;
    static constexpr size_t TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    MPSCQueue<TaskEventPointer*, 1024> q;

    std::vector<std::unique_ptr<TaskEventPointer>> storage;
    storage.reserve(TOTAL);

    for (size_t i = 0; i < TOTAL; ++i)
    {
        auto obj = std::make_unique<TaskEventPointer>();
        obj->id = i + 1;
        obj->payload = "tsan";
        storage.emplace_back(std::move(obj));
    }

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
                size_t index = p * ITEMS_PER_PRODUCER + i;

                if ((i % 97) == 0)
                {
                    std::this_thread::yield();
                }

                while (true)
                {
                    try
                    {
                        q.push(storage[index].get());
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

    std::vector<std::atomic<uint8_t>> seen(TOTAL + 1);

    for (auto& v : seen)
    {
        v.store(0, std::memory_order_relaxed);
    }

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

            ASSERT_GE(item->id, 1);
            ASSERT_LE(item->id, TOTAL);
            ASSERT_EQ(item->payload, "tsan");

            uint8_t expected = 0;

            ASSERT_TRUE(
                seen[item->id].compare_exchange_strong(
                    expected,
                    1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed
                )
            );

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

    for (size_t id = 1; id <= TOTAL; ++id)
    {
        ASSERT_EQ(seen[id].load(std::memory_order_relaxed), 1);
    }

    ASSERT_EQ(q.pop(), nullptr);
}