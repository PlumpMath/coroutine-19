// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_COROUTINE_HPP_
#define _COROUTINE_COROUTINE_HPP_

#include <core.hpp>

namespace coroutine
{

    struct Config
    {
        int stack_size;
        int unwind;
    };

    class Coroutine
    {
    public:
        typedef intptr_t (*routine_t)(Coroutine &, intptr_t);

    public:
        Coroutine(routine_t f);
        ~Coroutine();

        intptr_t resume(intptr_t data=0);
        intptr_t yield(intptr_t data=0);

        bool complete() const {
            return coroutine::complete(_impl.get());
        }

        static
        intptr_t wrapper(coroutine_t *, intptr_t data) {
            Coroutine *self =
                reinterpret_cast<Coroutine*>(data);
            self->_data = self->_f(*self, self->_data);
            return data;
        }

        // 
        // provide movable semantic
        // 

        Coroutine(Coroutine &other) {
            _impl = other._impl;
            other._impl = 0;

            _data = other._data;
            other._data = 0;

            _f = other._f;
            other._f = 0;
        }

        Coroutine &operator=(Coroutine &other) {
            if(this != &other)
            {
                this->~Coroutine();
                
                _impl = other._impl;
                other._impl = 0;

                _data = other._data;
                other._data = 0;

                _f = other._f;
                other._f = 0;
            }
            return *this;
        }

    private:
        Coroutine(const Coroutine&);
        Coroutine &operator=(const Coroutine&);

    private:
        coroutine_ptr _impl;
        intptr_t _data;
        routine_t _f;
    };

    Coroutine::Coroutine(Coroutine::routine_t f)
        : _data(0)
        , _f(f)
    {
        _impl = create(wrapper);
    }
    
    Coroutine::~Coroutine()
    {}

    intptr_t Coroutine::resume(intptr_t data)
    {
        this->_data = data;
        ::coroutine::resume(_impl, (intptr_t)this);
        return this->_data;
    }

    intptr_t Coroutine::yield(intptr_t data)
    {
        this->_data = data;
        ::coroutine::yield(_impl.get(), (intptr_t)this);
        return this->_data;
    }
        
}

#endif  // _COROUTINE_COROUTINE_HPP_
