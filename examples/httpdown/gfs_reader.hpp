#ifndef _GFS_THREAD_HPP_
#define _GFS_THREAD_HPP_

#include <tuple>
#include <thread>
#include <vector>

#include <appframe/circular_array_queue.hpp>

#include <coroutine-cpp/dispatcher.hpp>

namespace co = coroutine;

class GfsReader
{
public:
    GfsReader();
    ~GfsReader();

    char *
    read(uint64_t id,
	 co::coroutine_t *c,
	 int timeout,
	 const std::string &filename,
	 std::size_t length,
	 uint64_t offset);

    int poll();

    void release_data_array(char *da);
private:
    void thread_fun();
    void start();
    void stop();

    typedef std::tuple<uint64_t,    // id
                       std::string, // filename
                       std::size_t, // nread
                       uint64_t    // offset
                       > ReqItem;

    typedef std::tuple<uint64_t,    // id
                       char*
                       > RespItem;

    CircularArrayQueue<ReqItem> _inq;
    CircularArrayQueue<RespItem> _outq;
    co::Dispatcher<uint64_t> _dispatcher;
    bool _stop;
    std::thread _thread;
};

#endif  // _GFS_THREAD_HPP_
