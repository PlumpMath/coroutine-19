// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_SCEDULER_HPP_
#define _COROUTINE_SCEDULER_HPP_

#include "core.hpp"
#include <event/event2.h>
#include <boost/any.hpp>

namespace coroutine
{

    intptr_t wrapper(self_t, intptr_t data)
    {
        Args *args = get_data<Args>(self);
        args.f(data);
        return 0;
    }

    class Sceduler
    {
    public:
        Sceduler(struct event_base *evbase);
        ~Sceduler();

        
        void poll();



        coroutine_t create(function_t func,
                           std::size_t stacksize);
        intptr_t resume(const coroutine_t &f, intptr_t data);
        
        intptr_t yield(intptr_t data);
                        
        intptr_t start(function_t func, intptr_t data);
        
        template<typename Key>
        std::pair<intptr_t, bool>
        wait(const std::string &channel,
             const Key &key,
             long timeout_sec = 0,
             long timeout_usec = 0);

        template<typename Key>
        void notify(const std::string &channel,
                    const Key &key,
                    intptr_t data);

        void sleep(long sec);

        void usleep(long usec);

    protected:
        void _sleep(long sec, long usec);

    private:
        struct event_base *_evbase;
        self_t _running;
        //coroutine_list _list;
    };

    inline
    coroutine_t sceduler::create(function_t func,
                                std::size_t stacksize)
    {
        return create(func, stacksize);
    }

    inline
    intptr_t sceduler::start(function_t func, intptr_t data)
    {
        coroutine_t f = create(func);
        return this->resume(f, data);
    }


    template<typename Key>
    inline
    intptr_t sceduler::wait(const std::string &channel,
                            const Key &key,
                            long timeout_sec,
                            long timeout_usec)
    {
        typedef Dispatcher<Key> type;
        const std::string name = (channel + ":" +
                                  typeid(type).name());
        any_channel &any = _channel_map[name];

        if(any == NULL)
        {
            any = new type(_evbase);
        }
        type *channel = static_cast<type*>(any);
        return channel->wait_for(key, _running,
                                 timeout_sec,
                                 timeout_usec);
    }

    template<typename Key>
    inline
    std::size_t sceduler::notify(const std::string &channel,
                                 const Key &key,
                                 intptr_t data)
    {
        typedef Dispatcher<Key> type;
        const std::string name = (channel + ":" +
                                  typeid(type).name());
        any_channel &any = _channel_map[name];
        if(any == NULL)
            return 0;

        type *channel = static_cast<type*>(any);
        const std::size_t n = channel->dispatch(key, data);
        return n;
    }

}

#endif  // _COROUTINE_SCEDULER_HPP_
