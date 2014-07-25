#include <core.hpp>
#include <gtest/gtest.h>


namespace co = coroutine;

struct ZeroGuard
{
    int *_ptr;
    ZeroGuard(int *ptr) : _ptr(ptr) {}
    ~ZeroGuard() { *_ptr = 0; }
};

intptr_t test_no_data_transfered(intptr_t data)
{
    // do something
    for(int i=0; i<16; ++i)
    {
        i++;
    }
    return 0;
}

TEST(Core, no_data_transfered)
{
    co::coroutine_ptr f =
        co::create(test_no_data_transfered);

    EXPECT_TRUE(!co::is_complete(f));

    co::resume(f);

    EXPECT_TRUE(co::is_complete(f));
    co::destroy(f);
}

intptr_t test_transfer_data(intptr_t data)
{
    co::coroutine_ptr self = (co::coroutine_ptr)data;

    int n = co::yield(self);
    EXPECT_TRUE(n == 1);
    co::yield(self, 2);

    return 0;
}

TEST(Core, transfer_data)
{
    co::coroutine_ptr f =
        co::create(test_transfer_data);

    co::resume(f, (intptr_t)f);
    int n = co::resume(f, 1);
    EXPECT_TRUE(n == 2);
    
    co::destroy(f);
}

intptr_t test_unwind_stack(intptr_t data)
{
    co::coroutine_ptr self = (co::coroutine_ptr)data;
    int *ptr = (int*)co::yield(self);
    ZeroGuard guard(ptr);
    co::yield(self);
    return 0;
}

TEST(Core, unwind_stack)
{
    co::coroutine_ptr f = co::create(test_unwind_stack);
    co::resume(f, (intptr_t)f);

    int n = 1;
    co::resume(f, (intptr_t)&n);
    EXPECT_TRUE(n == 1);

    co::destroy(f);
    EXPECT_TRUE(n == 0);
}

intptr_t test_throw_std_exception(intptr_t data)
{
    throw std::bad_alloc();
    return 0;
}

TEST(Core, throw_std_exception)
{
    co::coroutine_ptr f = co::create(test_throw_std_exception);
    EXPECT_THROW(co::resume(f), co::exception_std);
    
    co::destroy(f);
}

intptr_t test_throw_unknown_exception(intptr_t data)
{
    throw "throw some non-std exception";
    return 0;
}

TEST(Core, throw_unknown_exception)
{
    co::coroutine_ptr f =
        co::create(test_throw_unknown_exception);
    EXPECT_THROW(co::resume(f), co::exception_unknown);

    co::destroy(f);
}


intptr_t test_no_rethrow(intptr_t data)
{
    throw std::bad_alloc();
    return 0;
}

TEST(Core, no_rethrow)
{
    co::coroutine_ptr f = co::create(test_no_rethrow, false);
    EXPECT_NO_THROW(co::resume(f));
    
    co::destroy(f);
}
/*
void echo(coroutine_ptr self)
{
    yield(self, self.data());
}
*/
intptr_t echo(intptr_t data)
{
    co::coroutine_ptr self = (co::coroutine_ptr)data;
    data = yield(self);
    while(true) data = yield(self, data);
}

TEST(Core, echo)
{
    co::coroutine_ptr f = co::create(echo);
    co::resume(f, (intptr_t)f);

    for(int i=0; i<128; ++i)
    {
        EXPECT_EQ(resume(f, i), i);
    }
}


