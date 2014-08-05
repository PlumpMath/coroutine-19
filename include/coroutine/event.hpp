// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_EVENT_HPP_
#define _COROUTINE_EVENT_HPP_

#include "core.hpp"

#include <event2/event.h>

namespace coroutine
{
    inline
    coroutine_t create(routine_t f, struct event_base *evbase)
    {
        coroutine_t c(create(f));
        set_event_base(c, evbase);
        return c;
    }

    inline
    coroutine_t create(routine_t f,
                       struct event_base *evbase,
                       std::size_t stacksize)
    {
        coroutine_t c(create(f, stacksize));
        set_event_base(c, evbase);
        return c;
    }

    void poll_event_base(struct event_base *evbase);

    void sleep(self_t c, long sec, long usec);

    inline
    void sleep(self_t c, long sec)
    {
        sleep(c, sec, 0);
    }

    inline
    void usleep(self_t c, long usec)
    {
        sleep(c, 0, usec);
    }

}

#endif  // _COROUTINE_EVENT_HPP_


