#include <event.hpp>

#include <memory>

#include <event2/event_struct.h>

namespace coroutine
{
    Event::Event()
    {
        _base = event_base_new();
        if(!_base) throw std::bad_alloc();
    }

    Event::~Event()
    {
        event_base_free(_base);
    }

    void Event::poll()
    {
        int flags = EVLOOP_NONBLOCK;
        int rv = event_base_loop(_base, flags);
        assert(rv != -1);
        (void)rv;
    }

    static
    void timeout_callback(evutil_socket_t fd,
                          short event,
                          void *arg)
    {
        coroutine_t *c = (coroutine_t*)arg;
        resume(c);
    }

    void Event::sleep(coroutine_t *c, long sec, long usec)
    {
        struct event timeout;
        struct timeval tv;

        evtimer_assign(&timeout, _base,
                       timeout_callback, (void*)c);
                       
        evutil_timerclear(&tv);
        tv.tv_sec = sec;
        tv.tv_usec = usec;
        event_add(&timeout, &tv);

        coroutine_ptr hold(c);  // prevent destroy
        yield(c);
    }

}
