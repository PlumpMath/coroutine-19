#include <dispatcher.hpp>

#include <string>

#include <gtest/gtest.h>

#include <event2/event.h>

#include "event.hpp"
namespace co = coroutine;

static 
struct event_base *base = event_base_new();

typedef co::Dispatcher<long> LongDispatcher;
typedef co::Dispatcher<std::string> StringDispatcher;

struct Guard
{
    Guard(std::string *s) : _s(s) {}
    ~Guard() { *_s = ""; }
    std::string *_s;
};

intptr_t wait_for_cat(co::self_t self, intptr_t data)
{
    StringDispatcher &d = *(StringDispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self, "cat");
    EXPECT_TRUE(ret.second);
    std::string &cat = *(std::string*)ret.first;
    EXPECT_EQ(cat, "this is a cat.");
    return 0;
}

intptr_t wait_for_dog(co::self_t self, intptr_t data)
{
    StringDispatcher &d = *(StringDispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self, "dog");
    EXPECT_TRUE(ret.second);
    std::string &dog = *(std::string*)ret.first;
    EXPECT_EQ(dog, "this is a dog.");
    return 0;
}

intptr_t wait_for_dog_dup(co::self_t self, intptr_t data)
{
    StringDispatcher &d = *(StringDispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self, "dog");
    EXPECT_FALSE(ret.second);
    return 0;
}

intptr_t wait_for_tiger(co::self_t self, intptr_t data)
{
    StringDispatcher &d = *(StringDispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self, "tiger");
    EXPECT_TRUE(ret.second);
    std::string &tiger = *(std::string*)ret.first;
    EXPECT_EQ(tiger, "this is a tiger.");
    return 0;
}

intptr_t wait_for_tiger_with_guard(co::self_t self,
                                   intptr_t data)
{
    StringDispatcher &d = *(StringDispatcher*)data;
    std::pair<intptr_t, bool> ret = d.wait(self, "tiger");
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
    StringDispatcher &d = *(StringDispatcher*)data;
    std::pair<intptr_t, bool> ret =
        d.wait(self, "tiger", 1);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(ret.first, 0);

    while(true) yield(self);

    return 0;
}

TEST(Dispatcher, dispatch_success)
{

    StringDispatcher d;

    {
        co::resume(co::create(wait_for_cat, base),
                   (intptr_t)&d);
        co::resume(co::create(wait_for_dog, base),
                   (intptr_t)&d);
        co::resume(co::create(wait_for_tiger, base),
                   (intptr_t)&d);
    }

    std::string data;
    std::size_t n;

    data = "this is a cat.";
    n = d.notify("cat", (intptr_t)&data);
    EXPECT_TRUE(n == 1);

    data = "this is a dog.";
    n = d.notify("dog", (intptr_t)&data);
    EXPECT_TRUE(n == 1);

    data = "this is a tiger.";
    n = d.notify("tiger", (intptr_t)&data);
    EXPECT_TRUE(n == 1);
}


TEST(Dispatcher, wait_for_duplicated)
{

    StringDispatcher d;

    {
        co::resume(co::create(wait_for_dog, base),
                   (intptr_t)&d);
        co::resume(co::create(wait_for_dog_dup, base),
                   (intptr_t)&d);
    }

    int n;
    std::string data = "this is a dog.";
    n = d.notify("dog", (intptr_t)&data);
    EXPECT_TRUE(n == 1);

    n = d.notify("dog", (intptr_t)&data);
    EXPECT_TRUE(n == 0);
}


TEST(Dispatcher, dispatch_no_waiters)
{

    StringDispatcher d;

    {
        co::resume(co::create(wait_for_dog, base),
                   (intptr_t)&d);
    }

    int n;
    std::string data = "this is a cat.";
    n = d.notify("cat", (intptr_t)&data);
    EXPECT_TRUE(n == 0);
}

TEST(Dispatcher, coroutine_auto_destroy)
{

    StringDispatcher d;

    {
        co::resume(co::create(wait_for_tiger_with_guard, base),
                   (intptr_t)&d);
    }

    int n;
    std::string data = "this is a tiger.";
    n = d.notify("tiger", (intptr_t)&data);
    EXPECT_TRUE(n == 1);
    EXPECT_EQ(data, "");
}

TEST(Dispatcher, wait_for_1s_timeout)
{

    StringDispatcher d;

    co::coroutine_t c = co::create(wait_for_1s_timeout, base);
    co::resume(c, (intptr_t)&d);

    event_base_dispatch(base);

    // here is no waiter already
    std::size_t n;
    std::string data = "this is a tiger.";
    n = d.notify("tiger", (intptr_t)&data);
    EXPECT_TRUE(n == 0);
}



