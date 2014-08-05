// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_SCEDULER_HPP_
#define _COROUTINE_SCEDULER_HPP_

#include <core.hpp>

#include <queue>

namespace coroutine
{

    class Sceduler
    {
    public:
        // use for new created coroutines
        void add(const coroutine_t &c);
        // use inner of coroutines
        void wait_for_scedule(self_t c);

        // run some coroutines
        int poll(int max_process_once = 64);

        bool empty() const { return _queue.empty(); }

    private:
        std::queue<coroutine_t> _queue;
    };

    inline
    void Sceduler::add(const coroutine_t &c)
    {
        _queue.push(c);
    }

    inline
    void Sceduler::wait_for_scedule(self_t c)
    {
        _queue.push(coroutine_t(c));
        coroutine::yield(c);
    }

}

#endif  // _COROUTINE_SCEDULER_HPP_


