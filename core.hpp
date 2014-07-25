// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_CORE_HPP_
#define _COROUTINE_CORE_HPP_

#include <stdint.h>
#include <exception>
#include <boost/intrusive_ptr.hpp>

namespace coroutine
{
    struct coroutine_t;

    typedef intptr_t (*routine_t)(coroutine_t *, intptr_t);
    typedef boost::intrusive_ptr<coroutine_t> coroutine_ptr;

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

    coroutine_ptr create(routine_t f, bool rethrow=true, int stack=64*1024);
    void destroy(coroutine_t *c);

    intptr_t resume(coroutine_t *c, intptr_t data=0);
    inline
    intptr_t resume(coroutine_ptr c, intptr_t data=0)
    { return resume(c.get(), data); }
    intptr_t yield(coroutine_t *c, intptr_t data=0);

    bool is_complete(coroutine_t *c);
        
}

#endif  // _COROUTINE_CORE_HPP_
