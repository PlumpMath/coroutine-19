
基于boost::context实现一个简单易用的协程库。

该库有以下特性：

1. 栈自动回溯。未执行完的协程释放时，会自动回溯栈，令C++对象能正确析构。
2. 协程生命周期自动管理。无需显式调用destroy，协程未在执行并没有任何地
   方持有时，会自动释放协程。

依赖库：

1. boost::context (-lboost_context)
2. boost::intrusive_ptr

待实现：

1. 实现队列上的等待
2. 栈使用mmap分配，并设置mprotect防止栈溢出


