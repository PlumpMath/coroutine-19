#ifndef _HTTP_SERVER_HPP_
#define _HTTP_SERVER_HPP_

#include <memory>

#include <gce/gfe/ihttpprocessor.hpp>

namespace co = coroutine;

class FixedSizeAllocator;
class HttpReactor;
class GfsReader;

class HttpServer : public IHttpProcessor
{
public:
    explicit
    HttpServer(int port,
               GfsReader *reader,
               struct event_base *evbase);
    ~HttpServer();

    int poll();

	void accept(HttpConnection &conn);
	void remove(HttpConnection &conn);
    void process_input(HttpConnection &conn,
                       ListByteBuffer* lbb);
private:
    typedef std::unique_ptr<FixedSizeAllocator> allocator_ptr;
    typedef std::unique_ptr<HttpReactor> reactor_ptr;
    allocator_ptr _allocator;
    reactor_ptr _reactor;
    GfsReader *_reader;
    struct event_base *_evbase;
};


#endif  // _HTTP_SERVER_HPP_


