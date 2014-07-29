#include <coroutine.hpp>

#include <core.hpp>
#include <gtest/gtest.h>


intptr_t echo_twice(coroutine::Coroutine &self, intptr_t data)
{
    return self.yield(data);
}

intptr_t echo_forever(coroutine::Coroutine &self, intptr_t data)
{
    while(true) data = self.yield(data);
    return 0;
}

TEST(Coroutine, normal)
{
    coroutine::Coroutine fecho(echo_twice);
    EXPECT_EQ(fecho.resume(1), 1);
    EXPECT_EQ(fecho.resume(2), 2);
    EXPECT_TRUE(fecho.complete());

    coroutine::Coroutine f2(echo_forever);
    fecho = f2;
    for(int i=0; i<128; ++i)
    {
        EXPECT_EQ(fecho.resume(i), i);
    }
}

