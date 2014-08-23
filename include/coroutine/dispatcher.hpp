#ifndef _COROUTINE_DISPATCHER_HPP_
#define _COROUTINE_DISPATCHER_HPP_

#include "core.hpp"

#include <unordered_set>

#include <event2/event.h>

namespace coroutine
{

    class Dispatcher
    {
    public:
        // 等待特定key上的数据通知。返回值的second代表是否成功。如果不
        // 成功，说明key已经有其它协程在等了；很有可能是key冲突了。如果已
        // 成功，则first是分发回来的数据。
        //
        // sec/usec是等待超时的时间，都不设置（都为0）代表永远等待。
        std::pair<intptr_t, bool>
        wait(self_t co, const struct timeval *tv = NULL);

        std::pair<intptr_t, bool>
        wait(self_t co, long sec) {
            struct timeval tv;
            tv.tv_sec = sec;
            tv.tv_usec = 0;
            return wait(co, &tv);
        }
        
        // 向target发送数据通知。
        // 返回激活的协程数。返回0，表示无协程在等待；
        std::size_t notify(self_t target, intptr_t data);

    private:
        std::unordered_set<self_t> _waitings;

    private:
        static
        void timeout_callback(evutil_socket_t fd,
                              short event,
                              void *arg);
    };

}

#endif  // _COROUTINE_DISPATCHER_HPP_
