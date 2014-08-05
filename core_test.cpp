#include <core.hpp>
#include <gtest/gtest.h>


namespace co = coroutine;

struct ZeroGuard
{
    int *_ptr;
    ZeroGuard(int *ptr) : _ptr(ptr) {}
    ~ZeroGuard() { *_ptr = 0; }
};

intptr_t test_no_data_transfered(co::self_t self, intptr_t data)
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
    co::coroutine_t f =
        co::create(test_no_data_transfered);

    EXPECT_TRUE(!co::complete(f));

    co::resume(f);

    EXPECT_TRUE(co::complete(f));
}

intptr_t test_transfer_data(co::self_t self, intptr_t data)
{
    int n = data;
    EXPECT_TRUE(n == 1);
    co::yield(self, 2);

    return 0;
}

TEST(Core, transfer_data)
{
    co::coroutine_t f =
        co::create(test_transfer_data);

    int n = co::resume(f, 1);
    EXPECT_TRUE(n == 2);
}

intptr_t test_unwind_stack(co::self_t self, intptr_t data)
{
    int *ptr = (int*)data;
    ZeroGuard guard(ptr);
    while(true) co::yield(self);
    return 0;
}

TEST(Core, unwind_stack)
{
    int n = 1;

    {
        co::coroutine_t f = co::create(test_unwind_stack);
        co::resume(f, (intptr_t)&n);
        EXPECT_TRUE(n == 1);
    }

    EXPECT_TRUE(n == 0);
}

intptr_t test_throw_std_exception(co::self_t self, intptr_t data)
{
    throw std::bad_alloc();
    return 0;
}

TEST(Core, throw_std_exception)
{
    co::coroutine_t f = co::create(test_throw_std_exception);
    EXPECT_THROW(co::resume(f), co::exception_std);
}

intptr_t test_throw_unknown_exception(co::self_t self, intptr_t data)
{
    throw "throw some non-std exception";
    return 0;
}

TEST(Core, throw_unknown_exception)
{
    co::coroutine_t f =
        co::create(test_throw_unknown_exception);
    EXPECT_THROW(co::resume(f), co::exception_unknown);
}


intptr_t test_no_rethrow(co::self_t self, intptr_t data)
{
    throw std::bad_alloc();
    return 0;
}

TEST(Core, no_rethrow)
{
    co::coroutine_t f = co::create(test_no_rethrow, false);
    EXPECT_NO_THROW(co::resume(f));
}

intptr_t test_no_holding_ptr(co::self_t self,
                             intptr_t data)
{
    int * run = (int*)data;
    ZeroGuard guard(run);
    while(*run)
    {
        yield(self);
    }
    // after unwind, *run will be zero
    return 0;
}

TEST(Core, no_holding_ptr)
{
    int run = 1;
    {
        co::resume(co::create(test_no_holding_ptr),
                   (intptr_t)&run);     // give run in
    }
    
    // should be destory at here because no chance to resume
    
    // 只有析构时，栈进行回溯后，run会被置为1。以此来判断是否析构和释放。
    EXPECT_TRUE(run == 0);
}


/*
void echo(coroutine_ptr self)
{
    yield(self, self.data());
}
*/
intptr_t echo(co::self_t self, intptr_t data)
{
    while(true) data = yield(self, data);
    return 0;
}

TEST(Core, echo)
{
    co::coroutine_t f = co::create(echo);

    for(int i=0; i<128; ++i)
    {
        EXPECT_EQ(resume(f, i), i);
    }
}

intptr_t echo_by_udata(co::self_t self, intptr_t data)
{
    while(true)
    {
        data = get_data(self);
        yield(self, data);
    }
    return 0;
}

TEST(Core, echo_by_udata)
{
    co::coroutine_t f = co::create(echo_by_udata);

    for(int i=0; i<128; ++i)
    {
        co::set_data(f, i);
        EXPECT_EQ(resume(f), i);
    }
}

intptr_t dummy(co::self_t self, intptr_t data)
{
    return 0;
}

void destory_callback(co::self_t self)
{
    std::string *str = co::get_data<std::string>(self);
    *str = "world";
}

TEST(Core, destory_callback)
{
    co::coroutine_t f = co::create(dummy);
    std::string str("hello");
    co::set_data(f, &str);
    co::set_destroy_callback(f, destory_callback);
    f.reset();                  // release and destroy
    EXPECT_TRUE(str == "world");
}

TEST(Core, set_get_name)
{
    co::coroutine_t f = co::create(dummy);
    co::set_name(f, "dummy");
    const std::string name = co::get_name(f);
    EXPECT_TRUE(name == "dummy");
}

TEST(Core, get_info)
{
    co::coroutine_t f = co::create(dummy);
    co::set_name(f, "dummy");
    std::string info = co::get_info(f);
    std::cout << info << std::endl;

    co::resume(f);
    EXPECT_TRUE(co::complete(f));

    info = co::get_info(f);
    std::cout << info << std::endl;
}

