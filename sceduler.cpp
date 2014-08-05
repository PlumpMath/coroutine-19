// -*-mode:c++; coding:utf-8-*-
#include <sceduler.hpp>

namespace coroutine
{
    //
    // 这里使用running先单独保存本次循环可运行的协程，再依次执行。这是
    // 为了避免在一轮调度中，重复调度同样的协程。比如协程resume后马上
    // 又等待在调度器上的情况。
    // 
    int Sceduler::poll(int max_process_once)
    {
        if(_queue.empty()) return 0;

        int n = 0;
        std::queue<coroutine_t> running;
        while(n<max_process_once && !_queue.empty())
        {
            ++n;
            running.push(_queue.front());
            _queue.pop();
        }

        while(! running.empty())
        {
            resume(running.front());
            running.pop();
        }

        return n;
    }
}

