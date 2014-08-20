// -*-mode:c++; coding:utf-8-*-
#include <coroutine/core.hpp>

#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <cstring>
#include <assert.h>
#include <new>

#include <boost/context/all.hpp>
namespace ctx = boost::context;

namespace coroutine
{
    
    enum flag_t
    {
        // （0-3比特组）协程运行状态：挂起<->运行->完成
        flag_complete = 1 << 0,     // 标识是否运行完
        flag_suspend = 1 << 1,  // 标识是否挂起状态
        // running == ! complete && ! suspend

        // （4-7比特组）协程选项值
        flag_unwind = 1 << 4,   // 回溯栈开关
        flag_rethrow = 1 << 5,  // 重抛异常开关

        // （8-11比特组）内部标识值
        flag_request_unwind_stack = 1 << 8, // 请求回溯栈
        flag_has_std_exception = 1 << 9,    // 有std异常抛出
        flag_has_unknown_exception = 1 << 10, // 有未知异常抛出
    };

    static const std::size_t MAX_NAME_LEN = 256;

    struct coroutine_impl_t
    {
        char name[MAX_NAME_LEN];
        int flags;
        routine_t f;
        intptr_t arg;
        intptr_t udata;
        intptr_t event_base;    // store struct event_base*
        void *context;
        void *caller;
        int refcount;
        destroy_callback destroy_cb;

        coroutine_impl_t() :
            flags(0), f(NULL), arg(0), udata(0), event_base(0),
            context(NULL), caller(NULL),
            refcount(0), destroy_cb(NULL)
            {}

        ~coroutine_impl_t()
            { if(destroy_cb) { destroy_cb(this); } }
        
    };

    struct forced_unwind {};

    static
    void routine_starter(intptr_t arg)
    {
        coroutine_impl_t *co(reinterpret_cast<coroutine_impl_t*>(arg));
        try
        { co->arg = co->f(co, co->arg); }
        catch (const forced_unwind &)
        {}
        catch (const std::exception &)
        { co->flags |= flag_has_std_exception; }
        catch (...)
        { co->flags |= flag_has_unknown_exception; }

        co->flags |= flag_complete;
        ctx::jump_fcontext((ctx::fcontext_t*)co->context,
                           (ctx::fcontext_t*)co->caller,
                           co->arg);
    }
        
    coroutine_t create(routine_t f, int stacksize, Options opt)
    {
        void *p = std::malloc(stacksize + sizeof(coroutine_impl_t));
        char *top = (char *)p + stacksize;
        // alloc coroutine at top of stack and the stack is growing
        // downward.
        coroutine_impl_t *co(new(top) coroutine_impl_t);
        co->flags |= flag_suspend;
        if(opt.unwind) co->flags |= flag_unwind;
        if(opt.rethrow) co->flags |= flag_rethrow;
        co->f = f;
        co->context = ctx::make_fcontext(top, stacksize,
                                         routine_starter);
        return coroutine_t(co);
    }

    static
    intptr_t resume(coroutine_impl_t *c, intptr_t arg=0);

    void destroy(coroutine_impl_t *c)
    {
        if(!(c->flags & flag_complete) &&
           (c->flags & flag_unwind))
        {
            c->flags |= flag_request_unwind_stack;
            resume(c);
        }
        // adjust pointer to head of memory
        ctx::fcontext_t *ctx = (ctx::fcontext_t*)c->context;
        void *p = (char*)c - ctx->fc_stack.size;
        c->~coroutine_impl_t();
        std::free(p);
    }

    static
    intptr_t resume(coroutine_impl_t *c, intptr_t arg)
    {
        assert((c->flags & flag_suspend) &&
               !(c->flags & flag_complete));
        c->flags &= ~flag_suspend;
        c->arg = arg;
        ctx::fcontext_t caller;
        c->caller = &caller;
        ctx::jump_fcontext((ctx::fcontext_t*)c->caller,
                           (ctx::fcontext_t*)c->context,
                           reinterpret_cast<intptr_t>(c));
        c->caller = NULL;

        if((c->flags & flag_rethrow) &&
           (c->flags & flag_has_std_exception))
            throw exception_std();
        if((c->flags & flag_rethrow) &&
           (c->flags & flag_has_unknown_exception))
            throw exception_unknown();
            
        return c->arg;
    }

    intptr_t resume(const coroutine_t &f, intptr_t arg)
    {
        // always hold the coroutine before jump into
        coroutine_t c(f); 
        return resume(c.get(), arg);
    }

    intptr_t yield(coroutine_impl_t* c, intptr_t arg)
    {
        assert(!(c->flags & flag_suspend) &&
               !(c->flags & flag_complete));
        c->flags |= flag_suspend;
        c->arg = arg;
        ctx::jump_fcontext((ctx::fcontext_t*)c->context,
                           (ctx::fcontext_t*)c->caller,
                           reinterpret_cast<intptr_t>(c));
        if(c->flags & flag_request_unwind_stack)
        {
            throw forced_unwind();
        }
        return c->arg;
    }

    bool complete(const coroutine_t &c)
    {
        return c->flags & flag_complete;
    }

    void set_data(const coroutine_t &c, intptr_t data)
    {
        c->udata = data;
    }

    intptr_t get_data(self_t c)
    {
        return c->udata;
    }

    void set_name(const coroutine_t &c, const char *name)
    {
        snprintf(c->name, MAX_NAME_LEN, "%s", name);
    }

    const char *get_name(const coroutine_t &c)
    {
        return c->name;
    }

    void set_destroy_callback(const coroutine_t &c,
                              destroy_callback cb)
    {
        c->destroy_cb = cb;
    }

    void intrusive_ptr_add_ref(coroutine_impl_t *p)
    {
        ++ p->refcount;
    }

    void intrusive_ptr_release(coroutine_impl_t *p)
    {
        -- p->refcount;
        if(p->refcount == 0)
            destroy(p);
    }

    void set_event_base(const coroutine_t &c,
                        struct event_base *evbase)
    {
        c->event_base = (intptr_t)evbase;
    }

    struct event_base *get_event_base(self_t c)
    {
        return (struct event_base *)c->event_base;
    }

    std::string get_info(const coroutine_t &c)
    {
        std::ostringstream oss;
        oss << "coroutine " << c.get()
            << " <" << c->name << "> ("
            << (void*)(intptr_t)c->flags << ")" // 输出十六进制值
            /*<< " flag-complete: "
            << ((c->flags & flag_complete) != 0)
            << " flag-suspend: "
            << ((c->flags & flag_suspend) != 0)
            << " flag-unwind: "
            << ((c->flags & flag_unwind) != 0)
            << " flag-rethrow: "
            << ((c->flags & flag_rethrow) != 0)
            << " flag-request-unwind-stack: "
            << ((c->flags & flag_request_unwind_stack) != 0)
            << " flag-has-std-exception: "
            << ((c->flags & flag_has_std_exception) != 0)
            << " flag-has-unknown-exception: "
            << ((c->flags & flag_has_unknown_exception) != 0)*/
            << " routine: " << (void*)c->f
            << " arg: " << c->arg << " (" << (void*)c->arg << ")"
            << " udata: " << c->udata << " (" << (void*)c->udata << ")"
            << " context: " << c->context
            << " caller: " << c->caller
            << " refcount: " << c->refcount
            << " destroy_cb: " << (void*)c->destroy_cb;
        return oss.str();
    }

}

