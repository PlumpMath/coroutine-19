// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_CORE_HPP_
#define _COROUTINE_CORE_HPP_

#include <stdint.h>
#include <exception>

namespace coroutine
{
    struct coroutine_t;

    typedef intptr_t (*routine_t)(intptr_t);
    typedef coroutine_t * coroutine_ptr;

    struct exception_unknown {};
    struct exception_std : public std::exception
    {
        virtual ~exception_std() throw() {}
        const char* what() const throw() {
            return "catch std::exception from coroutine";
        }
    };

    coroutine_ptr create(routine_t f, bool rethrow=true, int stack=64*1024);
    void destroy(coroutine_ptr c);

    intptr_t resume(coroutine_ptr c, intptr_t data=0);
    intptr_t yield(coroutine_ptr c, intptr_t data=0);

    bool is_complete(coroutine_ptr c);
        
}

#endif  // _COROUTINE_CORE_HPP_
