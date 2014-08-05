#include <event.hpp>

#include <memory>

#include <event2/event_struct.h>

namespace coroutine
{
    void poll_event_base(struct event_base *evbase)
    {
        int flags = EVLOOP_NONBLOCK;
        int rv = event_base_loop(evbase, flags);
        assert(rv != -1);
        (void)rv;
    }

    static
    void timeout_callback(evutil_socket_t fd,
                          short event,
                          void *arg)
    {
        self_t c = (self_t)arg;
        resume(c);
    }

    void sleep(self_t c, long sec, long usec)
    {
        struct event_base *evbase =
            (struct event_base *)get_event_base(c);
        struct event timeout;
        struct timeval tv;

        evtimer_assign(&timeout, evbase,
                       timeout_callback, (void*)c);
                       
        evutil_timerclear(&tv);
        tv.tv_sec = sec;
        tv.tv_usec = usec;
        event_add(&timeout, &tv);

        coroutine_t hold(c);  // prevent destroy
        yield(c);
    }

}
