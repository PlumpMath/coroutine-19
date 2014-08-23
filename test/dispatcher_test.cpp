#include <coroutine/dispatcher.hpp>

#include <string>

#include <gtest/gtest.h>

#include <event2/event.h>

#include <coroutine/event.hpp>
namespace co = coroutine;

static 
struct event_base *base = event_base_new();

struct Guard
{
    Guard(std::string *s) : _s(s) {}
    ~Guard() { *_s = ""; }
    std::string *_s;
};

intptr_t no_wait_f(co::self_t self, intptr_t data)
{
    while(true) co::sleep(self, 1);
    return 0;
}

intptr_t wait_for_cat(co::self_t self, intptr_t data)
{
    co::Dispatcher &d = *(co::Dispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self);
    EXPECT_TRUE(ret.second);
    std::string &cat = *(std::string*)ret.first;
    EXPECT_EQ(cat, "this is a cat.");
    return 0;
}

intptr_t wait_for_dog(co::self_t self, intptr_t data)
{
    co::Dispatcher &d = *(co::Dispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self);
    EXPECT_TRUE(ret.second);
    std::string &dog = *(std::string*)ret.first;
    EXPECT_EQ(dog, "this is a dog.");
    return 0;
}

intptr_t wait_for_dog_dup(co::self_t self, intptr_t data)
{
    co::Dispatcher &d = *(co::Dispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self);
    EXPECT_FALSE(ret.second);
    return 0;
}

intptr_t wait_for_tiger(co::self_t self, intptr_t data)
{
    co::Dispatcher &d = *(co::Dispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self);
    EXPECT_TRUE(ret.second);
    std::string &tiger = *(std::string*)ret.first;
    EXPECT_EQ(tiger, "this is a tiger.");
    return 0;
}

intptr_t wait_for_tiger_with_guard(co::self_t self,
                                   intptr_t data)
{
    co::Dispatcher &d = *(co::Dispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self);
    EXPECT_TRUE(ret.second);
    std::string *tiger = (std::string*)ret.first;
    Guard guard(tiger);
    EXPECT_EQ(*tiger, "this is a tiger.");

    while(true) co::yield(self);

    return 0;
}

intptr_t wait_for_1s_timeout(co::self_t self,
                             intptr_t data)
{
    co::Dispatcher &d = *(co::Dispatcher*)data;
    std::pair<intptr_t, bool> ret =
        d.wait(self, 1);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(ret.first, 0);

    while(true) yield(self);

    return 0;
}

TEST(Dispatcher, dispatch_success)
{

    co::Dispatcher d;

    co::self_t cat;
    co::self_t dog;
    co::self_t tiger;
    {
        co::coroutine_t c = co::create(wait_for_cat, base);
        cat = c.get();
        co::resume(c, (intptr_t)&d);

        c = co::create(wait_for_dog, base);
        dog = c.get();
        co::resume(c, (intptr_t)&d);

        c = co::create(wait_for_tiger, base);
        tiger = c.get();
        co::resume(c, (intptr_t)&d);
    }

    std::string data;
    std::size_t n;

    data = "this is a cat.";
    n = d.notify(cat, (intptr_t)&data);
    EXPECT_TRUE(n == 1);

    data = "this is a dog.";
    n = d.notify(dog, (intptr_t)&data);
    EXPECT_TRUE(n == 1);

    data = "this is a tiger.";
    n = d.notify(tiger, (intptr_t)&data);
    EXPECT_TRUE(n == 1);
}


TEST(Dispatcher, wait_for_duplicated)
{

    co::Dispatcher d;

    co::self_t dog;
    co::self_t dog_dup;
    {
        co::coroutine_t c = co::create(wait_for_dog, base);
        dog = c.get();
        co::resume(c, (intptr_t)&d);

        c = co::create(wait_for_dog_dup, base);
        dog_dup = c.get();
        co::resume(c, (intptr_t)&d);
    }

    int n;
    std::string data = "this is a dog.";
    n = d.notify(dog, (intptr_t)&data);
    EXPECT_TRUE(n == 1);

    n = d.notify(dog, (intptr_t)&data);
    EXPECT_TRUE(n == 0);
}


TEST(Dispatcher, dispatch_no_waiters)
{
    co::Dispatcher d;

    co::self_t no_wait = co::bad_coroutine;

    int n;
    n = d.notify(no_wait, (intptr_t)&d);
    EXPECT_TRUE(n == 0);

    {
        co::coroutine_t c = co::create(no_wait_f, base);
        no_wait = c.get();
        co::resume(c, (intptr_t)&d);
    }

    std::string data = "this is a cat.";
    n = d.notify(no_wait, (intptr_t)&data);
    EXPECT_TRUE(n == 0);
}

TEST(Dispatcher, coroutine_auto_destroy)
{

    co::Dispatcher d;

    co::self_t tiger;
    {
        co::coroutine_t c =
            co::create(wait_for_tiger_with_guard, base);
        tiger = c.get();
        co::resume(c, (intptr_t)&d);
    }

    int n;
    std::string data = "this is a tiger.";
    n = d.notify(tiger, (intptr_t)&data);
    EXPECT_TRUE(n == 1);
    EXPECT_EQ(data, "");
}

TEST(Dispatcher, wait_for_1s_timeout)
{
    struct event_base *base = event_base_new();
    co::Dispatcher d;

    co::coroutine_t c = co::create(wait_for_1s_timeout, base);
    co::self_t tiger = c.get();
    co::resume(c, (intptr_t)&d);

    event_base_dispatch(base);

    // here is no waiter already
    std::size_t n;
    std::string data = "this is a tiger.";
    n = d.notify(tiger, (intptr_t)&data);
    EXPECT_TRUE(n == 0);

    event_base_free(base);
}



