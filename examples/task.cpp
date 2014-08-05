// -*-mode:c++; coding:utf-8-*-
#include "task.hpp"

namespace coroutine
{
    sceduler::sceduler(struct event_base *evbase)
        : _evbase(evbase)
    {
    }

    sceduler::~sceduler()
    {
    }

    void sceduler::poll()
    {
        int flags = EVLOOP_NONBLOCK;
        int rv = event_base_loop(_base, flags);
        assert(rv != -1);
        (void)rv;
    }

    intptr_t sceduler::resume(const coroutine_t &f,
                              intptr_t data)
    {
        assert(_running == bad_coroutine);
        _running = f.get();
        
        intptr_t val = resume(f, data);
        _running = bad_coroutine;

        return val;
    }

    intptr_t sceduler::yield(intptr_t data)
    {
        self_t cur = _running;
        _running = bad_coroutine;
        
        intptr_t val = yield(cur);
        _running = cur;
        
        return val;
    }

    static
    void timeout_callback(evutil_socket_t fd,
                          short event,
                          void *arg)
    {
        self_t c = (self_t)arg;
        resume(c);
    }

    void sceduler::_sleep(long sec, long usec)
    {
        struct event timeout;
        struct timeval tv;

        evtimer_assign(&timeout, _base,
                       timeout_callback, (void*)_running);
                       
        evutil_timerclear(&tv);
        tv.tv_sec = sec;
        tv.tv_usec = usec;
        event_add(&timeout, &tv);

        coroutine_t hold(_running);  // prevent destroy
        this->yield();
    }

    struct WithEvent
    {
        WithEvent(event_base *base);

        sleep();
    };
    
}

