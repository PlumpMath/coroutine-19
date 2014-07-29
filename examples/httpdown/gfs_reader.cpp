#include "gfs_reader.hpp"

#include <algorithm>

//#include <gfs.hpp>
#include "localfs.hpp"

namespace fs = localfs;

GfsReader::GfsReader(struct event_base *base)
    : _inq(10240),
      _outq(1024),
      _dispatcher(base),
      _stop(false)
{
    start();
}

GfsReader::~GfsReader()
{
    stop();
}

GfsReader::dataarray_t *
GfsReader::read(uint64_t id,
		co::coroutine_t *c,
		int timeout,
		const std::string &filename,
		std::size_t length,
		uint64_t offset)
{
    int rv = _inq.push(
        std::make_tuple(id,
                        filename,
                        length,
                        offset));
    assert(rv == 0);
    intptr_t d;
    bool ok;
    std::tie(d, ok) = _dispatcher.wait_for(id, c, timeout);
    assert(ok);
    GfsReader::dataarray_t *da = (GfsReader::dataarray_t*)d;

    return da;
}

int GfsReader::poll()
{
  const std::size_t max_process_once = 64;
    std::size_t n = 0;
    std::size_t max_once = (std::min)(_outq.size(),
				      max_process_once);
    while(!_outq.empty() && (++n < max_once))
    {
        RespItem &item = _outq.front();
        _dispatcher.dispatch(std::get<0>(item),
                             (intptr_t)std::get<1>(item));

	_outq.pop();
    }

    return 0;
}

static
GfsReader::dataarray_t *read_file(const char *filename,
				  std::size_t length,
				  uint64_t offset)
                     
{
    fs::file_t fd = fs::open(filename);
    GfsReader::dataarray_t *data = new GfsReader::dataarray_t();
    data->reserve(length);
    ssize_t rv = fs::preadn(fd, &(*data)[0], length, offset);
    assert((size_t)rv == length);
    data->resize(length);
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
        dataarray_t *data = read_file(std::get<1>(req).c_str(),
                                      std::get<2>(req),
                                      std::get<3>(req));
	_inq.pop();
	
        RespItem resp = std::make_tuple(std::get<0>(req),
                                        data);
        _outq.push(resp);
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
