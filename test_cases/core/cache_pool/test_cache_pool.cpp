#include <gtest/gtest.h>
#include <cache/share_string.h>

TEST(CachePoolTest, SingleProducerSingleConsumerBasic)
{
    constexpr int ITERATIONS = 100000;
    std::vector<std::string> test_data;
    test_data.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; ++i)
    {
        test_data.emplace_back("Test string " + std::to_string(i));
    }

    std::vector<StringReference*> acquired;
    acquired.resize(ITERATIONS);
    std::atomic<uint32_t> acquire_index(0);

    // Producer thread: acquire and fill data
    std::thread producer([&]()
    {
        for (int i = 0; i < ITERATIONS; ++i)
        {
            auto* ref = StringPool::acquire();
            ASSERT_NE(ref, nullptr);

            ref->data = test_data[i];
            ref->retain();
            acquired[i] = ref;
            acquire_index.fetch_add(1, std::memory_order_release);
        }
    });

    // Consumer thread: verify and release
    uint32_t expected_index = 0;
    std::thread consumer([&]()
    {
        while (true)
        {
            uint32_t current_index = acquire_index.load(std::memory_order_acquire);
            if (current_index > expected_index)
            {
                for (uint32_t i = expected_index; i < current_index; ++i)
                {
                    auto* ref = acquired[i];
                    ASSERT_NE(ref, nullptr);
                    EXPECT_EQ(ref->data, test_data[i]);

                    if (ref->release() == 1)
                    {
                        StringPool::release(ref);
                    }
                }
                expected_index = current_index;
            }

            if (expected_index >= ITERATIONS)
            {
                break; // All data processed
            }
        }
    });

    producer.join();
    consumer.join();
}

TEST(CachePoolTest, MultiProducerMultiConsumer)
{
    size_t pool_size_before = StringPool::size();

    // Client threads
    constexpr int NUM_CLIENTS = 50;
    std::vector<std::thread> clients;
    for (int c = 0; c < NUM_CLIENTS; ++c)
    {
        clients.emplace_back([&]()
        {
            for (size_t i = 0; i < 10000; i++)
            {
                auto * ref = StringPool::acquire();
                ASSERT_NE(ref, nullptr);
                ref->data = "Client " + std::to_string(c) + " - String " + std::to_string(i);
                ref->retain();

                if (ref->release() == 1)
                {
                    StringPool::release(ref);
                }
            }
        });
    }

    for (auto& t : clients) t.join();

    size_t pool_size_after = StringPool::size();
    ASSERT_EQ(pool_size_before, pool_size_after);

    size_t head_after = StringPool::head();
    size_t tail_after = StringPool::tail();

    ASSERT_EQ(head_after, tail_after);
}
