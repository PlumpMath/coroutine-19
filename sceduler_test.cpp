#include <sceduler.hpp>
#include <gtest/gtest.h>

namespace co = coroutine;

intptr_t test_add_new_coroutine(co::self_t c, intptr_t)
{
    while(true)
    {
        int n = co::yield(c);
        // value from scedule should be 0
        EXPECT_TRUE(n == 0);
    }
    return 0;
}

TEST(Sceduler, add_new_coroutine)
{
    co::Sceduler sceduler;
    sceduler.add(co::create(test_add_new_coroutine));
    int n = sceduler.poll();
    EXPECT_TRUE(n == 1);
}

intptr_t test_wait_for_rescedule(co::self_t c,
                                 intptr_t data)
{
    co::Sceduler &sceduler = *(co::Sceduler*)data;
    for(int i=0; i<(70); ++i)
    {
        sceduler.wait_for_scedule(c);
    }
    co::yield(c);
    return 0;
}

TEST(Sceduler, wait_for_rescedule)
{
    co::coroutine_t c = co::create(test_wait_for_rescedule);

    co::Sceduler sceduler;
    int val = co::resume(c, (intptr_t)&sceduler);
    // val should be 0 because yield in sceduler
    EXPECT_TRUE(val == 0);

    EXPECT_FALSE(sceduler.empty());

    for(int i=0; i<70; ++i)
    {
        int n = sceduler.poll();
        EXPECT_EQ(n, 1);
    }
    
    EXPECT_TRUE(sceduler.empty());
}


