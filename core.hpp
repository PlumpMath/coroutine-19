// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_CORE_HPP_
#define _COROUTINE_CORE_HPP_

#include <stdint.h>
#include <exception>
#include <boost/intrusive_ptr.hpp>

namespace coroutine
{
    struct coroutine_t;

    typedef boost::intrusive_ptr<coroutine_t> coroutine_ptr;
    typedef coroutine_t *self_ptr;
    typedef intptr_t (*routine_t)(self_ptr, intptr_t);

    coroutine_ptr create(routine_t f,
                         bool rethrow=true,
                         bool unwind=true,
                         int stacksize=64*1024);
    inline
    coroutine_ptr create(routine_t f, int stacksize)
    { return create(f, true, true, stacksize); }

    intptr_t resume(coroutine_t *c, intptr_t data=0);
    inline
    intptr_t resume(coroutine_ptr c, intptr_t data=0)
    { return resume(c.get(), data); }
    intptr_t yield(coroutine_t *c, intptr_t data=0);

    bool is_complete(coroutine_t *c);


        
    void intrusive_ptr_add_ref(coroutine_t *p);
    void intrusive_ptr_release(coroutine_t *p);

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
