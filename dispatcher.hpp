#ifndef _COROUTINE_DISPATCHER_HPP_
#define _COROUTINE_DISPATCHER_HPP_

#include "core.hpp"

#include <unordered_map>

#include <event2/event.h>

namespace coroutine
{

    template<typename Key>
    class Dispatcher
    {
    public:
        // 等待特定key上的数据通知。返回值的second代表是否成功。如果不
        // 成功，说明key已经有其它协程在等了；很有可能是key冲突了。如果已
        // 成功，则first是分发回来的数据。
        //
        // sec/usec是等待超时的时间，都不设置（都为0）代表永远等待。
        std::pair<intptr_t, bool>
        wait(self_t co, Key key, long sec = 0, long usec = 0);
        
        // 向key上发送数据通知，将激活等待该key的协程。
        // 返回激活的协程数。返回0，表示无协程在等待；
        std::size_t notify(Key key, intptr_t data);

    protected:
        static
        void timeout_callback(evutil_socket_t fd,
                              short event,
                              void *arg);

    private:
        typedef std::pair<coroutine_t,
                          struct event *> value_type;
        typedef std::unordered_map<Key, value_type> map_type;
        map_type _waitings;
        typedef Dispatcher<Key> dispatcher_t;
        typedef std::tuple<dispatcher_t*,
                           coroutine_t,
                           Key*> callback_arg;
    };

    template<typename Key>
    void
    Dispatcher<Key>::timeout_callback(evutil_socket_t fd,
                                    short event,
                                    void *arg)
    {
        dispatcher_t *self;
        coroutine_t co;
        Key *key;
        std::tie(self, co, key)
            = *(callback_arg*)arg;
        typename dispatcher_t::map_type::iterator it =
            self->_waitings.find(*key);
        if(it == self->_waitings.end())
            return;

        self->_waitings.erase(it);

        resume(co);
    }

    template<typename Key>
    std::pair<intptr_t, bool>
    Dispatcher<Key>::wait(self_t co, Key key,
                          long sec, long usec)
    {
        value_type empty;
        std::pair<typename map_type::iterator, bool> rv
            = _waitings.insert(
                typename map_type::value_type(key, empty)
                );
        if(! rv.second)
            return std::pair<intptr_t, bool>(0, false);

        struct event *timeout = NULL;
        
        if(sec != 0 || usec != 0)
        {
            callback_arg arg =
                std::make_tuple(this, coroutine_t(co), &key);

            struct event_base *evbase =
                (struct event_base *)get_event_base(co);
            timeout = evtimer_new(evbase, timeout_callback,
                                  &arg);
            struct timeval tv;
            evutil_timerclear(&tv);
            tv.tv_sec = sec;
            tv.tv_usec = usec;
            evtimer_add(timeout, &tv);
        }

        rv.first->second.first = coroutine_t(co);
        rv.first->second.second = timeout;

        intptr_t val = yield(co);
        return std::pair<intptr_t, bool>(val, true);
    }

    template<typename Key>
    std::size_t Dispatcher<Key>::notify(Key key, intptr_t data)
    {
        typename map_type::iterator it = _waitings.find(key);
        if( it == _waitings.end())
            return 0;

        struct event *timeout = it->second.second;
        if(timeout != NULL)
        {
            int rv = evtimer_del(timeout);
            assert(rv == 0);
            (void)rv;
        }
        
        coroutine_t co(it->second.first);
        _waitings.erase(it);    // must erase before resume

        resume(co, data);

        return 1;
    }
}

#endif  // _COROUTINE_DISPATCHER_HPP_
