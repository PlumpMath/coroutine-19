// -*-mode:c++; coding:utf-8-*-
#include <cstdio>
#include <assert.h>

#include <core.hpp>

namespace co = coroutine;

void stepout()
{
        std::printf("stepout\n");
}

intptr_t f1(intptr_t data)
{
        co::coroutine_t self = (co::coroutine_t)data;

        intptr_t n = co::yield(self, 11);
        std::printf("step1, n=%ld\n", n);
        
        n = co::yield(self, 12);
        std::printf("step2, n=%ld\n", n);
        
        n = co::yield(self, 13);
        std::printf("step3, n=%ld\n", n);

        stepout();

        std::printf("step4, return 14\n");
        return 14;
}

int main(int argc, char **argv)
{
        std::printf("create\n");
        co::coroutine_t fc = co::create(f1);

        intptr_t n = co::resume(fc, (intptr_t)fc);
        std::printf("main1, n=%ld\n", n);

        n = co::resume(fc, 1);
        std::printf("main2, n=%ld\n", n);

        n = co::resume(fc, 2);
        std::printf("main3, n=%ld\n", n);

        n = co::resume(fc, 3);
        std::printf("main4, n=%ld\n", n);

        // n = co::resume(fc, 4);  // should be abort

        co::destroy(fc);
        std::printf("destroy\n");

        return 0;
}

