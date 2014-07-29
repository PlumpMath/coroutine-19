#include <gfs_thread.hpp>

//#include <gfs.hpp>
#include <localfs.hpp>

namespace fs = localfs;

GfsReader::GfsReader()
    : _stop(false)
{
    start();
}

GfsReader::~GfsReader()
{
    stop();
}

dataarray_t *
GfsReader::request_read_file(uint64_t id,
                             co::coroutine_t *c,
                             int timeout,
                             const std::string &filename,
                             uint64_t offset,
                             std::size_t length)
{
    int rv = _inq.push(
        std::make_tuple(id,
                        filename,
                        offset,
                        length));
    assert(rv == 0);
    intptr_t d = _dispatcher.wait_for(id, c, timeout);
    dataarray_t *da = (dataarray_t*)d;

    return da;
}

int GfsReader::poll()
{
    const int max_process_once = 64;
    int n = 0;
    while(!_outq.empty() && (n++ < max_process_once))
    {
        RespItem &item = _outq.front();
        _dispatcher.dispatch(std::get<0>(item),
                             (intptr_t)std::get<1>(item));
    }
}

static
dataarray_t *
GfsReader::read_file(const char *filename,
                     std::size_t length,
                     uint64_t offset)
                     
{
    fs::file_t fd = fs::open(filename);
    dataarray_t *data = new dataarray_t();
    data->resize(length);
    ssize_t rv = fs::preadn(fd, &(*data)[0], length, offset);
    (void)rv;
    return data;
}

void GfsReader::release_data_array(dataarray_t *da)
{
    delete da;
}

void GfsReader::thread_fun()
{
    while(! _stop)
    {
        while(_inq.empty()) usleep(20);

        ReqItem &req = _inq.front();
        dataarray_t *data = read_file(std::get<1>(req),
                                      std::get<2>(req),
                                      std::get<3>(req));
        RespItem resp = std::make_tuple(std::get<0>(req),
                                        data);
        _outq.push(resp);
    }
}

void GfsReader::start()
{
    _thread = std::thread(std::bind(thread_fun, this));
}

void GfsReader::stop()
{
    _stop=true;
    _thread.join();
}
