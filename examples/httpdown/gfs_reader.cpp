#include "gfs_reader.hpp"

#include <algorithm>
#include <errno.h>

#ifdef _FS_USE_GFS_
#include "gfs.hpp"
namespace fs = gfs;
#else
#include "localfs.hpp"
namespace fs = localfs;
#endif

GfsReader::GfsReader()
    : _inq(10240),
      _outq(1024),
      _dispatcher(),
      _stop(false)
{
    start();
}

GfsReader::~GfsReader()
{
    stop();
}

ssize_t
GfsReader::read(co::self_t c,
                int timeout,
                const std::string &filename,
		char *buffer,
                std::size_t length,
                uint64_t offset)
{
    int rv = _inq.push(
        std::make_tuple(c,
                        filename,
			buffer,
                        length,
                        offset));
    assert(rv == 0);
    intptr_t d;
    bool ok;
    std::tie(d, ok) = _dispatcher.wait(c, timeout);
    assert(ok);

    return (ssize_t)d;
}

int GfsReader::poll()
{
    const std::size_t max_process_once = 64;
    std::size_t n = 0;
    std::size_t max_once = (std::min)(_outq.size(),
                                      max_process_once);
    while(!_outq.empty() && (n++ < max_once))
    {
        RespItem &item = _outq.front();
        n = _dispatcher.notify(std::get<0>(item),
                               std::get<1>(item));
        assert(n == 1);

        _outq.pop();
    }

    return 0;
}

static
ssize_t read_file(const char *filename,
		  char *buffer,
		  std::size_t length,
		  uint64_t offset)
                     
{
    fs::file_t fd = fs::open(filename);
    ssize_t rv = fs::preadn(fd, buffer, length, offset);
    fs::close(fd);
    if((size_t)rv < length)
    {
        printf("read error, errno:%d, rv:%ld",
               fs::get_errno(), rv);
    }
    return rv;
}

void GfsReader::thread_fun()
{
    while(! _stop)
    {
        while(_inq.empty()) usleep(20);

        ReqItem &req = _inq.front();
        ssize_t nreaded = read_file(std::get<1>(req).c_str(),
				    std::get<2>(req),
				    std::get<3>(req),
				    std::get<4>(req));

        RespItem resp = std::make_tuple(std::get<0>(req),
                                        nreaded);
        _outq.push(resp);
        _inq.pop();
    }
}

void GfsReader::start()
{
    _thread = std::thread(std::bind(&GfsReader::thread_fun, this));
}

void GfsReader::stop()
{
    _stop=true;
    _thread.join();
}
