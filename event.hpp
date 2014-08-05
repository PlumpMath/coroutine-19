// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_EVENT_HPP_
#define _COROUTINE_EVENT_HPP_

#include "core.hpp"

#include <event2/event.h>

namespace coroutine
{
    // TODO: rename to EventWithCoroutine
    class Event
    {
    public:
        Event(struct event_base *base);
        ~Event();

        void poll();

        void sleep(self_t c, long sec)
            { sleep(c, sec, 0); }
        void usleep(self_t c, long usec)
            { sleep(c, 0, usec); }



        void accept();
        void connect();
        void send();
        void recv();
        void close();

    protected:
        void sleep(self_t c, long sec, long usec);

    private:
        struct event_base *_base;
    };

}

#endif  // _COROUTINE_EVENT_HPP_


