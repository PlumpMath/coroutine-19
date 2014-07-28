#ifndef _COROUTINE_DISPATCHER_HPP_
#define _COROUTINE_DISPATCHER_HPP_

#include <core.hpp>

#include <unordered_map>

#include <event2/event.h>

namespace coroutine
{

    template<typename T>
    class Dispatcher
    {
    public:
        explicit
        Dispatcher(struct event_base *base)
            : _base(base) {}

        // 等待某个ID相关数据的分发。返回值的second代表是否成功。如果不
        // 成功，说明ID已经有其它协程在等了；很有可能是ID冲突了。如果已
        // 成功，则first是分发回来的数据。
        //
        // sec/usec是等待超时的时间
        std::pair<intptr_t, bool>
        wait_for(T id, coroutine_t *co,
                 long sec = 0, long usec = 0);
        
        // 分发一个ID，将激活等待该ID的协程。返回激活的协程数。
        std::size_t dispatch(T id, intptr_t data);

    protected:
        static
        void timeout_callback(evutil_socket_t fd,
                              short event,
                              void *arg);

    private:
        struct event_base *_base;
        typedef std::pair<coroutine_ptr, struct event *> value_type;
        typedef std::unordered_map<T, value_type> map_type;
        map_type _waitings;
    };

    // template<typename T>
    // std::pair<intptr_t, bool>
    // Dispatcher<T>::wait_for(T id, coroutine_t *co)
    // {
    //     std::pair<intptr_t, bool> ret(0, false);
    //     ret.second = _waitings.insert(
    //         typename map_type::value_type(id,
    //                                       coroutine_ptr(co))
    //         ).second;
    //     if(ret.second)
    //         ret.first = yield(co);
    //     return ret;
    // }

    template<typename T>
    struct callback_arg
    {
        Dispatcher<T> *dispatcher;
        coroutine_t *co;
        T *id;
    };

    template<typename T>
    void
    Dispatcher<T>::timeout_callback(evutil_socket_t fd,
                                    short event,
                                    void *arg)
    {
        callback_arg<T> &cb = *(callback_arg<T>*)arg;
        typename Dispatcher<T>::map_type::iterator it =
            cb.dispatcher->_waitings.find(*cb.id);
        if(it == cb.dispatcher->_waitings.end())
            return;

        cb.dispatcher->_waitings.erase(it);

        resume(cb.co);
    }

    template<typename T>
    std::pair<intptr_t, bool>
    Dispatcher<T>::wait_for(T id, coroutine_t *co,
                            long sec, long usec)
    {
        value_type empty(coroutine_ptr(), NULL);
        std::pair<typename map_type::iterator, bool> rv
            = _waitings.insert(
                typename map_type::value_type(id, empty)
                );
        if(! rv.second)
            return std::pair<intptr_t, bool>(0, false);

        struct event *timeout = NULL;

        callback_arg<T> arg = { this, co, &id };
        
        if(sec != 0 || usec != 0)
        {
            timeout = evtimer_new(_base, timeout_callback,
                                  &arg);
            struct timeval tv;
            evutil_timerclear(&tv);
            tv.tv_sec = sec;
            tv.tv_usec = usec;
            evtimer_add(timeout, &tv);
        }

        rv.first->second.first = coroutine_ptr(co);
        rv.first->second.second = timeout;

        intptr_t val = yield(co);
        return std::pair<intptr_t, bool>(val, true);
    }

    template<typename T>
    std::size_t Dispatcher<T>::dispatch(T id, intptr_t data)
    {
        typename map_type::iterator it = _waitings.find(id);
        if( it == _waitings.end())
            return 0;

        coroutine_ptr co(it->second.first);
        struct event *timeout = it->second.second;
        if(timeout != NULL)
        {
            int rv = evtimer_del(timeout);
            assert(rv == 0);
        }
        
        _waitings.erase(it);    // must erase before resume

        resume(co, data);

        return 1;
    }
}

#endif  // _COROUTINE_DISPATCHER_HPP_
