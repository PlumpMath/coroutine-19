// -*-mode:c++; coding:utf-8-*-
#include <core.hpp>

#include <cstdlib>
#include <cstring>
#include <assert.h>

#include <boost/context/all.hpp>
namespace ctx = boost::context;

namespace coroutine
{
    
    enum status_t
    {
        S_COMPLETE = 0,
        S_SUSPEND = 1,
        S_RUNNING = 2
    };

    struct coroutine
    {
        int status;
        char *sp;
        int ssize;
        routine_t f;
        intptr_t data;
        void *context;
        void *caller;
    };

    static
    void routine_starter(intptr_t data)
    {
        coroutine_t co = reinterpret_cast<coroutine_t>(data);
        co->data = co->f(co->data);
        co->status = S_COMPLETE;
        ctx::jump_fcontext((ctx::fcontext_t*)co->context,
                           (ctx::fcontext_t*)co->caller,
                           co->data);
    }
        
    coroutine_t create(routine_t f, int stack)
    {
        void *p = std::malloc(stack + sizeof(coroutine));
        coroutine_t co = reinterpret_cast<coroutine_t>(p);
        std::memset(co, 0, sizeof(*co));
        co->status = S_SUSPEND;
        co->sp = (char*)(co + 1) + stack;
        co->ssize = stack;
        co->f = f;
        co->context = ctx::make_fcontext(co->sp, co->ssize,
                                         routine_starter);
        return co;
    }

    void destroy(coroutine_t c)
    {
        assert(c->status != S_RUNNING);
        std::free(c);
    }

    intptr_t resume(coroutine_t c, intptr_t data)
    {
        assert(c->status == S_SUSPEND);
        c->status = S_RUNNING;
        c->data = data;
        ctx::fcontext_t caller;
        c->caller = &caller;
        ctx::jump_fcontext((ctx::fcontext_t*)c->caller,
                           (ctx::fcontext_t*)c->context,
                           reinterpret_cast<intptr_t>(c));
        return c->data;
    }

    intptr_t yield(coroutine_t c, intptr_t data)
    {
        assert(c->status == S_RUNNING);
        c->status = S_SUSPEND;
        c->data = data;
        ctx::jump_fcontext((ctx::fcontext_t*)c->context,
                           (ctx::fcontext_t*)c->caller,
                           reinterpret_cast<intptr_t>(c));
        return c->data;
    }

    bool is_complete(coroutine_t c)
    {
        return c->status == S_COMPLETE;
    }

}

