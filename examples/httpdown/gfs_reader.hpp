#ifndef _GFS_THREAD_HPP_
#define _GFS_THREAD_HPP_

#include <tuple>
#include <thread>

#include <circular_array_queue.hpp>

#include <coroutine-cpp/dispatcher.hpp>

class GfsReader
{
public:
    typedef std::vector<char> dataarray_t;

public:
    GfsReader();
    ~GfsReader();

    dataarray_t *
    request_read_file(uint64_t id,
                      co::coroutine_t *c,
                      int timeout,
                      const std::string &filename,
                      uint64_t offset,
                      std::size_t length);

    int poll();

    void release_data_array(dataarray_t *da);
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
                       dataarray_t*
                       > RespItem;

    CircularArrayQueue<ReqItem> _inq;
    CircularArrayQueue<RespItem> _outq;
    co::Dispatcher<uint64_t> _dispatcher;
    bool _stop;
    std::thread _thread;
};

#endif  // _GFS_THREAD_HPP_
