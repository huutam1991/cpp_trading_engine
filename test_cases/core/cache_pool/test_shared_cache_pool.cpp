#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include <cache/shared_cache_pool.h>

using StringPool = SharedCachePool<std::string, 20000>;
using StringObject = StringPool::ObjectPointer;

// =========================
// ObjectPointer tests
// =========================

TEST(SharedCachePoolStringTest, ObjectPointerAcquireNotNull)
{
    auto obj = StringPool::acquire();

    ASSERT_NE(obj.object, nullptr);
    ASSERT_NE(obj.reference_counter, nullptr);
    EXPECT_EQ(obj.reference_counter->load(std::memory_order_acquire), 1);
}

TEST(SharedCachePoolStringTest, ObjectPointerCanWriteString)
{
    auto obj = StringPool::acquire();

    *obj.object = "hello";
    EXPECT_EQ(*obj.object, "hello");
}

TEST(SharedCachePoolStringTest, ObjectPointerCopyIncrementsReferenceCounter)
{
    auto obj1 = StringPool::acquire();

    {
        StringObject obj2 = obj1;
        EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 2);
        EXPECT_EQ(obj2.reference_counter->load(std::memory_order_acquire), 2);
    }

    EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 1);
}

TEST(SharedCachePoolStringTest, ObjectPointerMultipleCopiesIncrementCorrectly)
{
    auto obj1 = StringPool::acquire();
    StringObject obj2 = obj1;
    StringObject obj3 = obj1;

    EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 3);
}

TEST(SharedCachePoolStringTest, ObjectPointerCopySharesSameObject)
{
    auto obj1 = StringPool::acquire();
    *obj1.object = "shared";

    StringObject obj2 = obj1;

    EXPECT_EQ(obj1.object, obj2.object);
    EXPECT_EQ(*obj2.object, "shared");

    *obj2.object = "changed";
    EXPECT_EQ(*obj1.object, "changed");
}

TEST(SharedCachePoolStringTest, ObjectPointerMoveTransfersOwnership)
{
    auto obj1 = StringPool::acquire();
    auto* raw_object = obj1.object;
    auto* raw_counter = obj1.reference_counter;

    StringObject obj2 = std::move(obj1);

    EXPECT_EQ(obj1.object, nullptr);
    EXPECT_EQ(obj1.reference_counter, nullptr);

    EXPECT_EQ(obj2.object, raw_object);
    EXPECT_EQ(obj2.reference_counter, raw_counter);
    EXPECT_EQ(obj2.reference_counter->load(std::memory_order_acquire), 1);
}

TEST(SharedCachePoolStringTest, ObjectPointerCopyAssignmentIncrementsReferenceCounter)
{
    auto obj1 = StringPool::acquire();
    auto obj2 = StringPool::acquire();

    obj2 = obj1;

    EXPECT_EQ(obj1.object, obj2.object);
    EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 2);
}

TEST(SharedCachePoolStringTest, ObjectPointerMoveAssignmentTransfersOwnership)
{
    auto obj1 = StringPool::acquire();
    auto obj2 = StringPool::acquire();

    auto* raw_object = obj1.object;
    auto* raw_counter = obj1.reference_counter;

    obj2 = std::move(obj1);

    EXPECT_EQ(obj1.object, nullptr);
    EXPECT_EQ(obj1.reference_counter, nullptr);

    EXPECT_EQ(obj2.object, raw_object);
    EXPECT_EQ(obj2.reference_counter, raw_counter);
}

TEST(SharedCachePoolStringTest, ObjectPointerSelfCopyAssignmentSafe)
{
    auto obj = StringPool::acquire();

    auto* raw_object = obj.object;
    auto* raw_counter = obj.reference_counter;

    obj = obj;

    EXPECT_EQ(obj.object, raw_object);
    EXPECT_EQ(obj.reference_counter, raw_counter);
    EXPECT_EQ(obj.reference_counter->load(std::memory_order_acquire), 1);
}

TEST(SharedCachePoolStringTest, ObjectPointerSelfMoveAssignmentSafe)
{
    auto obj = StringPool::acquire();

    auto* raw_object = obj.object;
    auto* raw_counter = obj.reference_counter;

    obj = std::move(obj);

    EXPECT_EQ(obj.object, raw_object);
    EXPECT_EQ(obj.reference_counter, raw_counter);
    EXPECT_EQ(obj.reference_counter->load(std::memory_order_acquire), 1);
}

// =========================
// Pool behavior tests
// =========================

TEST(SharedCachePoolStringTest, AcquireReducesPoolSize)
{
    size_t before = StringPool::size();

    {
        auto obj = StringPool::acquire();
        EXPECT_EQ(StringPool::size(), before - 1);
    }

    EXPECT_EQ(StringPool::size(), before);
}

TEST(SharedCachePoolStringTest, ReleaseRestoresPoolSize)
{
    size_t before = StringPool::size();

    {
        auto obj = StringPool::acquire();
    }

    EXPECT_EQ(StringPool::size(), before);
}

TEST(SharedCachePoolStringTest, ReleasedStringIsRefreshedToEmpty)
{
    {
        auto obj = StringPool::acquire();
        *obj.object = "temporary data";
    }

    auto obj2 = StringPool::acquire();

    EXPECT_TRUE(obj2.object->empty());
}

TEST(SharedCachePoolStringTest, MultipleAcquireReducesSizeCorrectly)
{
    size_t before = StringPool::size();

    auto obj1 = StringPool::acquire();
    auto obj2 = StringPool::acquire();
    auto obj3 = StringPool::acquire();

    EXPECT_EQ(StringPool::size(), before - 3);
}

TEST(SharedCachePoolStringTest, MultipleReleaseRestoresSizeCorrectly)
{
    size_t before = StringPool::size();

    {
        auto obj1 = StringPool::acquire();
        auto obj2 = StringPool::acquire();
        auto obj3 = StringPool::acquire();
    }

    EXPECT_EQ(StringPool::size(), before);
}

TEST(SharedCachePoolStringTest, TotalReleasedItemsIncreasesAfterAcquire)
{
    size_t before = StringPool::total_released_items();

    auto obj = StringPool::acquire();

    EXPECT_EQ(StringPool::total_released_items(), before + 1);
}

TEST(SharedCachePoolStringTest, TotalReleasedItemsRestoresAfterRelease)
{
    size_t before = StringPool::total_released_items();

    {
        auto obj = StringPool::acquire();
    }

    EXPECT_EQ(StringPool::total_released_items(), before);
}

TEST(SharedCachePoolStringTest, HeadMovesAfterAcquire)
{
    size_t before_head = StringPool::head();

    auto obj = StringPool::acquire();

    EXPECT_NE(StringPool::head(), before_head);
}

TEST(SharedCachePoolStringTest, TailMovesAfterRelease)
{
    size_t before_tail = StringPool::tail();

    {
        auto obj = StringPool::acquire();
    }

    EXPECT_NE(StringPool::tail(), before_tail);
}

TEST(SharedCachePoolStringTest, ReleaseOnlyAfterAllCopiesDestroyed)
{
    size_t before = StringPool::size();

    {
        auto obj1 = StringPool::acquire();
        EXPECT_EQ(StringPool::size(), before - 1);

        {
            StringObject obj2 = obj1;
            StringObject obj3 = obj1;

            EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 3);
            EXPECT_EQ(StringPool::size(), before - 1);
        }

        EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 1);
        EXPECT_EQ(StringPool::size(), before - 1);
    }

    EXPECT_EQ(StringPool::size(), before);
}

TEST(SharedCachePoolStringTest, MoveDoesNotIncreaseReferenceCounterAndReleaseOnce)
{
    size_t before = StringPool::size();

    {
        auto obj1 = StringPool::acquire();
        auto* counter = obj1.reference_counter;

        EXPECT_EQ(counter->load(std::memory_order_acquire), 1);

        StringObject obj2 = std::move(obj1);

        EXPECT_EQ(obj1.reference_counter, nullptr);
        EXPECT_EQ(counter->load(std::memory_order_acquire), 1);
        EXPECT_EQ(StringPool::size(), before - 1);
    }

    EXPECT_EQ(StringPool::size(), before);
}

TEST(SharedCachePoolStringTest, CopyThenMoveThenReleaseAfterLastOwner)
{
    size_t before = StringPool::size();

    {
        auto obj1 = StringPool::acquire();

        StringObject obj2 = obj1;
        EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 2);

        StringObject obj3 = std::move(obj2);

        EXPECT_EQ(obj2.reference_counter, nullptr);
        EXPECT_EQ(obj1.reference_counter->load(std::memory_order_acquire), 2);
        EXPECT_EQ(StringPool::size(), before - 1);
    }

    EXPECT_EQ(StringPool::size(), before);
}

TEST(SharedCachePoolStringTest, ConcurrentAcquireReleaseBasicStress)
{
    constexpr size_t THREAD_COUNT = 8;
    constexpr size_t LOOP_COUNT = 1000;

    size_t before = StringPool::size();

    std::vector<std::thread> threads;

    for (size_t t = 0; t < THREAD_COUNT; ++t)
    {
        threads.emplace_back([]()
        {
            for (size_t i = 0; i < LOOP_COUNT; ++i)
            {
                auto obj = StringPool::acquire();
                *obj.object = "test";
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(StringPool::size(), before);
}

TEST(SharedCachePoolStringTest, ConcurrentSharedObjectCopyMoveAcrossThreads)
{
    constexpr size_t THREAD_COUNT = 8;
    constexpr size_t LOOP_COUNT = 1000;

    size_t before = StringPool::size();

    {
        auto shared_obj = StringPool::acquire();
        *shared_obj.object = "initial";

        EXPECT_EQ(StringPool::size(), before - 1);
        EXPECT_EQ(shared_obj.reference_counter->load(std::memory_order_acquire), 1);

        std::vector<std::thread> threads;
        threads.reserve(THREAD_COUNT);

        for (size_t t = 0; t < THREAD_COUNT; ++t)
        {
            // copy into thread capture => refcount +1
            threads.emplace_back([shared_obj, t]()
            {
                for (size_t i = 0; i < LOOP_COUNT; ++i)
                {
                    // copy local => refcount +1
                    StringObject local_copy = shared_obj;

                    EXPECT_NE(local_copy.object, nullptr);
                    EXPECT_NE(local_copy.reference_counter, nullptr);

                    // move local => refcount unchanged
                    StringObject local_moved = std::move(local_copy);

                    EXPECT_EQ(local_copy.object, nullptr);
                    EXPECT_EQ(local_copy.reference_counter, nullptr);

                    EXPECT_NE(local_moved.object, nullptr);
                    EXPECT_NE(local_moved.reference_counter, nullptr);

                    // read/write shared object
                    // WARNING: std::string write itself is not thread-safe.
                    // For safety, only write deterministic same value rarely or use mutex.
                    if (i == 0)
                    {
                        *local_moved.object = "thread_" + std::to_string(t);
                    }

                    EXPECT_GE(
                        local_moved.reference_counter->load(std::memory_order_acquire),
                        2
                    );
                }
            });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        // All thread copies destroyed.
        // Only shared_obj should remain.
        EXPECT_EQ(shared_obj.reference_counter->load(std::memory_order_acquire), 1);
        EXPECT_EQ(StringPool::size(), before - 1);
    }

    // shared_obj destroyed, object released back to pool.
    EXPECT_EQ(StringPool::size(), before);
}