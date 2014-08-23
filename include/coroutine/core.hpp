// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_CORE_HPP_
#define _COROUTINE_CORE_HPP_

#include <stdint.h>
#include <string>
#include <exception>
#include <boost/intrusive_ptr.hpp>

// prototype
struct event_base;

namespace coroutine
{
    struct coroutine_impl_t;

    // coroutine_t - 用于保存创建的协程，可对其调用resume以执行协程
    typedef boost::intrusive_ptr<coroutine_impl_t> coroutine_t;

    // self_t - 用于协程内部，表示协程自己，可对其调用yield以返回到协程
    // 调用点
    typedef coroutine_impl_t *self_t;

    // routine_t - 可创建为协程的例程类型
    typedef intptr_t (*routine_t)(self_t, intptr_t);

    // bad_coroutine - 常量，表示错误协程。可与self_t或coroutine_t进行
    // ==或!=比较。
    #define bad_coroutine ((self_t)NULL);

    struct Options
    {
        Options() : rethrow(true), unwind(true) {}
        void enable_rethrow() { rethrow = true; }
        void disable_rethrow() { rethrow = false; }
        void enable_unwind() { unwind = true; }
        void disable_unwind() { unwind = false; }
        
        bool rethrow;
        bool unwind;
    };

    // create - 从例程创建一个协程出来
    //
    // rethrow - 表示内部异常是否要再次抛出来，如果为false，内部异常将
    // 导致协程退出，但不会抛出到resume之外。
    // unwind - 表示是否要自动回溯栈
    // stacksize - 协程栈大小，为0时使用系统栈大小，不允许小于4K将导致
    //
    // 出错时，返回bad_coroutine
    coroutine_t create(routine_t f,
                       int stacksize,
                       Options opt);

    // create - 重载版本，指定栈大小时调用这个比较方便
    inline
    coroutine_t create(routine_t f, int stacksize=64*1024)
    {
        Options opt;            // use default value
        return create(f, stacksize, opt);
    }

    // resume - 进入到协程的上次返回点开始执行，第一次是从头执行
    //
    // c - 协程
    // arg - 传给协程的数据，首次调用协程时，会通过参数传递；否则，通
    // 过yield返回值传递
    intptr_t resume(const coroutine_t &c, intptr_t arg=0);

    inline
    intptr_t resume(const coroutine_t &c, void *arg)
    {
        return resume(c, (intptr_t)arg);
    }

    // yield - 暂停协程执行，并返回到调用点
    //
    // c - 协程自己 
    // arg - 传回给调用点的数据，通过resume返回值传递
    intptr_t yield(self_t c, intptr_t arg=0);

    inline
    intptr_t yield(self_t c, void *arg)
    {
        return yield(c, (intptr_t)arg);
    }

    // 判断协程是否已执行完，即运行完了整个例程函数体或return
    bool complete(const coroutine_t &c);

    void set_data(const coroutine_t &c, intptr_t data);
    intptr_t get_data(self_t c);

    inline
    void set_data(const coroutine_t &c, void *data)
    {
        set_data(c, reinterpret_cast<intptr_t>(data));
    }
    template<typename T>
    inline
    T *get_data(self_t c)
    {
        return reinterpret_cast<T*>(get_data(c));
    }

    void set_name(const coroutine_t &c, const char *name);
    const char *get_name(const coroutine_t &c);

    std::string get_info(const coroutine_t &c);
    
    // 协程释放时调用的函数
    typedef void (*destroy_callback)(self_t);

    void set_destroy_callback(const coroutine_t &c,
                              destroy_callback cb);
        
    void intrusive_ptr_add_ref(coroutine_impl_t *p);
    void intrusive_ptr_release(coroutine_impl_t *p);

    void set_event_base(const coroutine_t &c,
                        struct event_base *evbase);
    struct event_base *get_event_base(self_t c);

    inline
    coroutine_t create(routine_t f, struct event_base *evbase)
    {
        coroutine_t c(create(f));
        set_event_base(c, evbase);
        return c;
    }

    inline
    coroutine_t create(routine_t f,
                       struct event_base *evbase,
                       std::size_t stacksize)
    {
        coroutine_t c(create(f, stacksize));
        set_event_base(c, evbase);
        return c;
    }

    struct exception_unknown {};

    struct exception_std : public std::exception
    {
        virtual ~exception_std() throw() {}
        const char* what() const throw() {
            return "catch std::exception from coroutine";
        }
    };
}

#endif  // _COROUTINE_CORE_HPP_
