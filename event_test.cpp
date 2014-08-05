#include <event.hpp>

#include <ctime>
#include <sys/time.h>
#include <gtest/gtest.h>

namespace co = coroutine;

co::Event *cur_event;

intptr_t sleep_1s(co::self_t c, intptr_t data)
{
    bool *stop = (bool*)data;

    std::time_t begin = std::time(NULL);
    cur_event->sleep(c, 1);
    std::time_t end = time(NULL);
    EXPECT_EQ(end - begin, 1);

    *stop = true;

    return 0;
}

intptr_t usleep_1500us(co::self_t c, intptr_t data)
{
    bool *stop = (bool*)data;

    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    cur_event->usleep(c, 1500);
    gettimeofday(&end, NULL);
    
    long elapsed =
        ((end.tv_sec * 1000 * 1000 + end.tv_usec) -
         (begin.tv_sec * 1000 * 1000 + begin.tv_usec));
    long diff = 70;
    EXPECT_TRUE((1500 - diff < elapsed) &&
                (1500 + diff > elapsed));

    *stop = true;

    return 0;
}

TEST(Event, sleep_1s)
{
    struct event_base *base = event_base_new();
    co::Event event(base);
    cur_event = &event;

    co::coroutine_t c(co::create(sleep_1s));

    std::time_t begin = std::time(NULL);

    bool stop = false;
    co::resume(c, (intptr_t)&stop);
    while(! stop)
    {
        event.poll();
    }
    
    std::time_t end = time(NULL);
    EXPECT_EQ(end - begin, 1);

}

TEST(Event, usleep_1500s)
{
    struct event_base *base = event_base_new();
    co::Event event(base);
    cur_event = &event;

    co::coroutine_t c(co::create(usleep_1500us));

    struct timeval begin, end;
    gettimeofday(&begin, NULL);
    
    bool stop = false;
    co::resume(c, (intptr_t)&stop);
    while(! stop)
    {
        event.poll();
    }
    
    gettimeofday(&end, NULL);
    long elapsed =
        ((end.tv_sec * 1000 * 1000 + end.tv_usec) -
         (begin.tv_sec * 1000 * 1000 + begin.tv_usec));
    long diff = 70;
    EXPECT_TRUE((1500 - diff < elapsed) &&
                (1500 + diff > elapsed));
}

