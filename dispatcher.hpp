#ifndef _COROUTINE_DISPATCHER_HPP_
#define _COROUTINE_DISPATCHER_HPP_

#include <core.hpp>

#include <map>

namespace coroutine
{

    template<typename T>
    class Dispatcher
    {
    public:
        // 等待某个ID相关数据的分发。返回值的second代表是否成功。如果不
        // 成功，说明ID已经有其它协程在等了；很有可能是ID冲突了。如果已
        // 成功，则first是分发回来的数据。
        std::pair<intptr_t, bool>
        wait_for(T id, coroutine_t *co);
        
        // 分发一个ID，将激活等待该ID的协程。返回激活的协程数。
        std::size_t dispatch(T id, intptr_t data);

    private:
        typedef std::map<T, coroutine_ptr> map_type;
        map_type _waitings;
    };

    template<typename T>
    std::pair<intptr_t, bool>
    Dispatcher<T>::wait_for(T id, coroutine_t *co)
    {
        std::pair<intptr_t, bool> ret(0, false);
        ret.second = _waitings.insert(
            typename map_type::value_type(id,
                                          coroutine_ptr(co))
            ).second;
        if(ret.second)
            ret.first = yield(co);
        return ret;
    }

    template<typename T>
    std::size_t Dispatcher<T>::dispatch(T id, intptr_t data)
    {
        typename map_type::iterator it = _waitings.find(id);
        if( it == _waitings.end())
            return 0;

        coroutine_ptr co(it->second);
        _waitings.erase(it);    // must erase before resume

        resume(co, data);

        return 1;
    }
}

#endif  // _COROUTINE_DISPATCHER_HPP_
