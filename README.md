
基于boost::context实现一个简单易用的协程库。

该库有以下特性：

1. 栈自动回溯。未执行完的协程释放时，会自动回溯栈，令C++对象能正确析构。
2. 协程生命周期自动管理。无需显式调用destroy，协程未在执行并没有任何地
   方持有时，会自动释放协程。
3. 基于libevent实现同步方式的网络操作接口，使用简单高效

常用组件：

1. 简单的调度器，轮循方式
2. 简单的分发器，适用于协程内需要等待接收一个数据的情况

依赖库：

1. boost::context (-lboost_context)
2. boost::intrusive_ptr
3. libevent

待实现：

1. 栈使用mmap分配，并设置mprotect防止栈溢出
2. 将所有coroutine用双向链表串起来，并提供遍历输出接口
3. 支持使用函数对象，这样方便在对象中存储更多的信息，避免要传入多个参数
   时的麻烦。
   （取消，weak\_ptr不能直接获得指针，为使用带来麻烦）
4. 使用std::function<intptr(coroutine_ptr, intptr)>替换原始函数类型，
   以支持函数对象
5. parse_ip_port: add "*:port" support

