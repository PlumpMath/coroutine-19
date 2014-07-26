// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_EVENT_HPP_
#define _COROUTINE_EVENT_HPP_

#include <core.hpp>

#include <event2/event.h>

namespace coroutine
{
    class Event
    {
    public:
        Event();
        ~Event();

        void poll();

        void sleep(coroutine_t *c, int sec)
            { sleep(c, sec, 0); }
        void usleep(coroutine_t *c, int usec)
            { sleep(c, 0, usec); }

    protected:
        void sleep(coroutine_t *c, int sec, int usec);

    private:
        struct event_base *_base;
        
    };

}

#endif  // _COROUTINE_EVENT_HPP_


