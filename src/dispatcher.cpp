#include <coroutine/dispatcher.hpp>

#include <tuple>

#include <event2/event_struct.h>

namespace coroutine
{

    typedef std::unordered_set<self_t>::iterator iterator_type;

    void Dispatcher::timeout_callback(evutil_socket_t fd,
                                      short event,
                                      void *arg)
    {
        Dispatcher *dispatcher;
        self_t co;
        std::tie(dispatcher, co)
            = *(std::tuple<Dispatcher*, self_t>*)arg;
        iterator_type it =
            dispatcher->_waitings.find(co);
        if(it == dispatcher->_waitings.end())
            return;

        dispatcher->_waitings.erase(it);

        resume(co);
    }

    std::pair<intptr_t, bool>
    Dispatcher::wait(self_t self,
                     const struct timeval *tv)
    {
        iterator_type it;
        bool ok = false;
        std::tie(it, ok) = _waitings.insert(self);
        if(! ok)
            return std::pair<intptr_t, bool>(0, false);

        coroutine_t hold(self);
        intptr_t val = 0;
        if(tv && evutil_timerisset(tv))
        {
            std::tuple<Dispatcher*, self_t> arg =
                std::make_tuple(this, self);

            struct event timeout;
            evtimer_assign(&timeout,
                           get_event_base(self),
                           timeout_callback,
                           &arg);
            evtimer_add(&timeout, tv);
            val = yield(self);
            evtimer_del(&timeout);
        }
        else
        {
            val = yield(self);
        }

        return std::pair<intptr_t, bool>(val, true);
    }

    std::size_t Dispatcher::notify(self_t target,
                                   intptr_t data)
    {
        iterator_type it = _waitings.find(target);
        if(it == _waitings.end())
            return 0;

        _waitings.erase(it);    // must erase before resume
        resume(target, data);

        return 1;
    }

}
