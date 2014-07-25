// -*-mode:c++; coding:utf-8-*-
#include <core.hpp>

#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <new>

#include <boost/context/all.hpp>
namespace ctx = boost::context;

namespace coroutine
{
    
    enum flag_t
    {
        // 协程运行状态：挂起<->运行->完成
        flag_complete = 1 << 1,     // 标识是否运行完
        flag_suspend = 1 << 2,  // 标识是否挂起状态
        // running == ! complete && ! suspend

        // 协程选项值
        flag_unwind = 1 << 3,   // 回溯栈开关
        flag_rethrow = 1 << 4,  // 重抛异常开关

        // 内部标识值
        flag_request_unwind_stack = 1 << 5, // 请求回溯栈
        flag_has_std_exception = 1 << 7,    // 有std异常抛出
        flag_has_unknown_exception = 1 << 8, // 有未知异常抛出
    };

    struct coroutine_t
    {
        int flags;
        routine_t f;
        intptr_t data;
        void *context;
        void *caller;
        int refcount;

        coroutine_t() :
            flags(0), f(NULL), data(0),
            context(NULL), caller(NULL),
            refcount(0)
            {}

        
    };

    struct forced_unwind {};

    static
    void routine_starter(intptr_t data)
    {
        coroutine_t *co(reinterpret_cast<coroutine_t*>(data));
        try
        { co->data = co->f(co, co->data); }
        catch (const forced_unwind &)
        {}
        catch (const std::exception &)
        { co->flags |= flag_has_std_exception; }
        catch (...)
        { co->flags |= flag_has_unknown_exception; }

        co->flags |= flag_complete;
        ctx::jump_fcontext((ctx::fcontext_t*)co->context,
                           (ctx::fcontext_t*)co->caller,
                           co->data);
    }
        
    coroutine_ptr create(routine_t f, bool rethrow, bool unwind, int stack)
    {
        void *p = std::malloc(stack + sizeof(coroutine_t));
        char *top = (char *)p + stack;
        // alloc coroutine at top of stack and the stack is growing
        // downward.
        coroutine_ptr co(new(top) coroutine_t);
        co->flags |= flag_suspend;
        if(unwind) co->flags |= flag_unwind;
        if(rethrow) co->flags |= flag_rethrow;
        co->f = f;
        co->context = ctx::make_fcontext(top, stack,
                                         routine_starter);
        return co;
    }

    void destroy(coroutine_t *c)
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
        c->~coroutine_t();
        std::free(p);
    }

    intptr_t resume(coroutine_t *c, intptr_t data)
    {
        assert((c->flags & flag_suspend) &&
               !(c->flags & flag_complete));
        c->flags &= ~flag_suspend;
        c->data = data;
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
            
        return c->data;
    }

    intptr_t yield(coroutine_t* c, intptr_t data)
    {
        assert(!(c->flags & flag_suspend) &&
               !(c->flags & flag_complete));
        c->flags |= flag_suspend;
        c->data = data;
        ctx::jump_fcontext((ctx::fcontext_t*)c->context,
                           (ctx::fcontext_t*)c->caller,
                           reinterpret_cast<intptr_t>(c));
        if(c->flags & flag_request_unwind_stack)
        {
            throw forced_unwind();
        }
        return c->data;
    }

    bool is_complete(coroutine_t *c)
    {
        return c->flags & flag_complete;
    }

    void intrusive_ptr_add_ref(coroutine_t *p)
    { ++ p->refcount; }

    void intrusive_ptr_release(coroutine_t *p)
    { if(-- p->refcount == 0) destroy(p); }

}

