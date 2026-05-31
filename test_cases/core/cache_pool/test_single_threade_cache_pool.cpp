#include <gtest/gtest.h>

#include <cache/single_thread_cache_pool.h>

#include <array>
#include <cstddef>

using Pool = SingleThreadCachePool<std::size_t>;

TEST(SingleThreadCachePoolTest, InitialState)
{
    Pool pool;

    ASSERT_TRUE(pool.full());
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE);
    ASSERT_EQ(pool.active_size(), 0);
}

TEST(SingleThreadCachePoolTest, AcquireOneObject)
{
    Pool pool;

    auto* object = pool.acquire();

    ASSERT_NE(object, nullptr);
    ASSERT_TRUE(object->is_active);
    ASSERT_EQ(object->index, 0);
    ASSERT_EQ(object->use_time, 1);

    ASSERT_FALSE(pool.full());
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE - 1);
    ASSERT_EQ(pool.active_size(), 1);
}

TEST(SingleThreadCachePoolTest, AcquireThreeObjectsSequentialIndex)
{
    Pool pool;

    auto* a = pool.acquire();
    auto* b = pool.acquire();
    auto* c = pool.acquire();

    ASSERT_EQ(a->index, 0);
    ASSERT_EQ(b->index, 1);
    ASSERT_EQ(c->index, 2);

    ASSERT_EQ(a->use_time, 1);
    ASSERT_EQ(b->use_time, 1);
    ASSERT_EQ(c->use_time, 1);

    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE - 3);
    ASSERT_EQ(pool.active_size(), 3);
}

TEST(SingleThreadCachePoolTest, ReleaseNullptrReturnsFalse)
{
    Pool pool;

    ASSERT_FALSE(pool.release(nullptr));
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE);
    ASSERT_EQ(pool.active_size(), 0);
}

TEST(SingleThreadCachePoolTest, ReleaseActiveObject)
{
    Pool pool;

    auto* object = pool.acquire();

    ASSERT_TRUE(pool.release(object));
    ASSERT_FALSE(object->is_active);

    ASSERT_TRUE(pool.full());
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE);
    ASSERT_EQ(pool.active_size(), 0);
}

TEST(SingleThreadCachePoolTest, ReleaseSameObjectTwiceReturnsFalse)
{
    Pool pool;

    auto* object = pool.acquire();

    ASSERT_TRUE(pool.release(object));
    ASSERT_FALSE(pool.release(object));

    ASSERT_TRUE(pool.full());
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE);
    ASSERT_EQ(pool.active_size(), 0);
}

TEST(SingleThreadCachePoolTest, AcquireAfterReleaseDoesNotImmediatelyReuseBecauseFifo)
{
    Pool pool;

    auto* first = pool.acquire();

    ASSERT_EQ(first->index, 0);
    ASSERT_TRUE(pool.release(first));

    auto* second = pool.acquire();

    ASSERT_NE(second, first);
    ASSERT_EQ(second->index, 1);
    ASSERT_EQ(second->use_time, 1);
}

TEST(SingleThreadCachePoolTest, ReleasedObjectIsReusedOnlyAfterRingWrapsAround)
{
    Pool pool;

    auto* first = pool.acquire();
    ASSERT_EQ(first->index, 0);

    ASSERT_TRUE(pool.release(first));

    for (std::size_t i = 1; i < MAX_POOL_SIZE; ++i)
    {
        auto* object = pool.acquire();

        ASSERT_NE(object, nullptr);
        ASSERT_EQ(object->index, i);
        ASSERT_EQ(object->use_time, 1);
    }

    auto* reused = pool.acquire();

    ASSERT_EQ(reused, first);
    ASSERT_EQ(reused->index, 0);
    ASSERT_EQ(reused->use_time, 2);
    ASSERT_TRUE(reused->is_active);

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.available_size(), 0);
    ASSERT_EQ(pool.active_size(), MAX_POOL_SIZE);
}

TEST(SingleThreadCachePoolTest, IsActiveReturnsTrueForActiveId)
{
    Pool pool;

    auto* object = pool.acquire();
    auto id = object->get_id();

    ASSERT_TRUE(pool.is_active(id));
}

TEST(SingleThreadCachePoolTest, IsActiveReturnsFalseAfterRelease)
{
    Pool pool;

    auto* object = pool.acquire();
    auto id = object->get_id();

    ASSERT_TRUE(pool.release(object));

    ASSERT_FALSE(pool.is_active(id));
}

TEST(SingleThreadCachePoolTest, OldIdBecomesInvalidAfterObjectIsReused)
{
    Pool pool;

    auto* first = pool.acquire();
    auto old_id = first->get_id();

    ASSERT_TRUE(pool.release(first));

    for (std::size_t i = 1; i < MAX_POOL_SIZE; ++i)
    {
        ASSERT_NE(pool.acquire(), nullptr);
    }

    auto* reused = pool.acquire();
    auto new_id = reused->get_id();

    ASSERT_EQ(reused, first);
    ASSERT_NE(old_id.use_time, new_id.use_time);

    ASSERT_FALSE(pool.is_active(old_id));
    ASSERT_TRUE(pool.is_active(new_id));
}

TEST(SingleThreadCachePoolTest, InvalidIndexIdReturnsFalse)
{
    Pool pool;

    Pool::Id id
    {
        MAX_POOL_SIZE,
        1
    };

    ASSERT_FALSE(pool.is_active(id));
}

TEST(SingleThreadCachePoolTest, WrongUseTimeReturnsFalse)
{
    Pool pool;

    auto* object = pool.acquire();

    Pool::Id wrong_id
    {
        object->index,
        object->use_time + 1
    };

    ASSERT_FALSE(pool.is_active(wrong_id));
}

TEST(SingleThreadCachePoolTest, AcquireUntilEmpty)
{
    Pool pool;

    for (std::size_t i = 0; i < MAX_POOL_SIZE; ++i)
    {
        auto* object = pool.acquire();

        ASSERT_NE(object, nullptr);
        ASSERT_TRUE(object->is_active);
        ASSERT_EQ(object->index, i);
        ASSERT_EQ(object->use_time, 1);
    }

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(pool.full());
    ASSERT_EQ(pool.available_size(), 0);
    ASSERT_EQ(pool.active_size(), MAX_POOL_SIZE);
}

TEST(SingleThreadCachePoolTest, AcquireWhenEmptyReturnsNullptr)
{
    Pool pool;

    for (std::size_t i = 0; i < MAX_POOL_SIZE; ++i)
    {
        ASSERT_NE(pool.acquire(), nullptr);
    }

    ASSERT_EQ(pool.acquire(), nullptr);

    ASSERT_TRUE(pool.empty());
    ASSERT_EQ(pool.available_size(), 0);
    ASSERT_EQ(pool.active_size(), MAX_POOL_SIZE);
}

TEST(SingleThreadCachePoolTest, ReleaseAllObjectsMakesPoolFullAgain)
{
    Pool pool;

    std::array<Pool::Object*, MAX_POOL_SIZE> objects{};

    for (std::size_t i = 0; i < MAX_POOL_SIZE; ++i)
    {
        objects[i] = pool.acquire();
        ASSERT_NE(objects[i], nullptr);
    }

    for (std::size_t i = 0; i < MAX_POOL_SIZE; ++i)
    {
        ASSERT_TRUE(pool.release(objects[i]));
    }

    ASSERT_TRUE(pool.full());
    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE);
    ASSERT_EQ(pool.active_size(), 0);
}

TEST(SingleThreadCachePoolTest, ReleasedObjectsAreReusedInFifoOrderAfterPoolExhaustion)
{
    Pool pool;

    std::array<Pool::Object*, 3> released_objects{};

    released_objects[0] = pool.acquire();
    released_objects[1] = pool.acquire();
    released_objects[2] = pool.acquire();

    ASSERT_EQ(released_objects[0]->index, 0);
    ASSERT_EQ(released_objects[1]->index, 1);
    ASSERT_EQ(released_objects[2]->index, 2);

    ASSERT_TRUE(pool.release(released_objects[0]));
    ASSERT_TRUE(pool.release(released_objects[1]));
    ASSERT_TRUE(pool.release(released_objects[2]));

    for (std::size_t i = 3; i < MAX_POOL_SIZE; ++i)
    {
        auto* object = pool.acquire();
        ASSERT_NE(object, nullptr);
        ASSERT_EQ(object->index, i);
    }

    auto* a = pool.acquire();
    auto* b = pool.acquire();
    auto* c = pool.acquire();

    ASSERT_EQ(a, released_objects[0]);
    ASSERT_EQ(b, released_objects[1]);
    ASSERT_EQ(c, released_objects[2]);

    ASSERT_EQ(a->use_time, 2);
    ASSERT_EQ(b->use_time, 2);
    ASSERT_EQ(c->use_time, 2);
}

TEST(SingleThreadCachePoolTest, PartialReleaseKeepsCorrectCounts)
{
    Pool pool;

    auto* a = pool.acquire();
    auto* b = pool.acquire();
    auto* c = pool.acquire();
    auto* d = pool.acquire();

    ASSERT_TRUE(pool.release(b));
    ASSERT_TRUE(pool.release(d));

    ASSERT_TRUE(a->is_active);
    ASSERT_FALSE(b->is_active);
    ASSERT_TRUE(c->is_active);
    ASSERT_FALSE(d->is_active);

    ASSERT_EQ(pool.active_size(), 2);
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE - 2);
}

TEST(SingleThreadCachePoolTest, PartialReleasedObjectsAreReusedAfterFreshObjects)
{
    Pool pool;

    auto* a = pool.acquire();
    auto* b = pool.acquire();
    auto* c = pool.acquire();
    auto* d = pool.acquire();

    ASSERT_TRUE(pool.release(b));
    ASSERT_TRUE(pool.release(d));

    for (std::size_t i = 4; i < MAX_POOL_SIZE; ++i)
    {
        auto* object = pool.acquire();
        ASSERT_NE(object, nullptr);
        ASSERT_EQ(object->index, i);
    }

    auto* reused_b = pool.acquire();
    auto* reused_d = pool.acquire();

    ASSERT_EQ(reused_b, b);
    ASSERT_EQ(reused_d, d);

    ASSERT_EQ(reused_b->use_time, 2);
    ASSERT_EQ(reused_d->use_time, 2);

    ASSERT_TRUE(a->is_active);
    ASSERT_TRUE(c->is_active);
    ASSERT_TRUE(reused_b->is_active);
    ASSERT_TRUE(reused_d->is_active);
}

TEST(SingleThreadCachePoolTest, ObjectValueCanBeWrittenAndRead)
{
    Pool pool;

    auto* object = pool.acquire();

    object->value = 123456;

    ASSERT_EQ(object->value, 123456);
}

TEST(SingleThreadCachePoolTest, ValuePersistsUntilObjectIsReused)
{
    Pool pool;

    auto* first = pool.acquire();

    first->value = 777;

    ASSERT_TRUE(pool.release(first));

    for (std::size_t i = 1; i < MAX_POOL_SIZE; ++i)
    {
        ASSERT_NE(pool.acquire(), nullptr);
    }

    auto* reused = pool.acquire();

    ASSERT_EQ(reused, first);
    ASSERT_EQ(reused->value, 777);
    ASSERT_EQ(reused->use_time, 2);
}

TEST(SingleThreadCachePoolTest, StressAcquireReleaseOneObjectWithFullRingTraversal)
{
    Pool pool;

    constexpr std::size_t CYCLE_COUNT = 10;

    for (std::size_t cycle = 0; cycle < CYCLE_COUNT; ++cycle)
    {
        auto* object = pool.acquire();

        ASSERT_NE(object, nullptr);

        std::size_t expected_index = cycle % MAX_POOL_SIZE;

        ASSERT_EQ(object->index, expected_index);
        ASSERT_EQ(object->use_time, 1);

        ASSERT_TRUE(pool.release(object));
    }

    ASSERT_TRUE(pool.full());
    ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE);
    ASSERT_EQ(pool.active_size(), 0);
}

TEST(SingleThreadCachePoolTest, StressFullPoolAcquireReleaseCycles)
{
    Pool pool;

    constexpr std::size_t CYCLE_COUNT = 20;

    std::array<Pool::Object*, MAX_POOL_SIZE> objects{};

    for (std::size_t cycle = 0; cycle < CYCLE_COUNT; ++cycle)
    {
        for (std::size_t i = 0; i < MAX_POOL_SIZE; ++i)
        {
            objects[i] = pool.acquire();

            ASSERT_NE(objects[i], nullptr);
            ASSERT_TRUE(objects[i]->is_active);
            ASSERT_EQ(objects[i]->index, i);
            ASSERT_EQ(objects[i]->use_time, cycle + 1);
        }

        ASSERT_TRUE(pool.empty());
        ASSERT_EQ(pool.acquire(), nullptr);

        for (std::size_t i = 0; i < MAX_POOL_SIZE; ++i)
        {
            ASSERT_TRUE(pool.release(objects[i]));
        }

        ASSERT_TRUE(pool.full());
        ASSERT_EQ(pool.available_size(), MAX_POOL_SIZE);
        ASSERT_EQ(pool.active_size(), 0);
    }
}