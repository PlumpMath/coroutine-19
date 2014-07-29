#include "http_server.hpp"

#include <tuple>
#include <algorithm>

#include <gce/gfe/http_reactor.h>
#include <appframe/fixed_size_allocator.h>

//#include "gfs.hpp"
#include "localfs.hpp"
namespace fs = localfs;

#include "gfs_reader.hpp"

typedef std::tuple<HttpConnection*,
                   GfsReader*,
		   co::Event*> arg_t;

HttpServer::HttpServer(int port, GfsReader *reader)
    : _reader(reader)
{
    const int page_size = 4*1024;
    const int mem_size = 40*1024;
    const char *server_name = "nginx/1.2.7";

    _allocator = allocator_ptr(new FixedSizeAllocator(
                                   page_size, page_size,
                                   mem_size));
    if(_allocator->init() < 0)
        throw std::runtime_error("init allocator fail");
    
    _reactor = reactor_ptr(new HttpReactor(_allocator.get(),
                                           this, port));
    if(_reactor->init(server_name) < 0)
        throw std::runtime_error("init reactor fail");

    _event = event_ptr(new co::Event());
}

HttpServer::~HttpServer()
{
}

int HttpServer::poll()
{
  _event->poll();
  int n= _reactor->poll();
  while ( n > 0)
    {
      n = _reactor->poll();
    }
return n;
}

void HttpServer::accept(HttpConnection &conn)
{
    conn.set_keep_alive(30);
    printf("accept conn %ld\n", conn.id());
}

void HttpServer::remove(HttpConnection &conn)
{
  printf("remove conn %ld\n", conn.id());
}

static
intptr_t process_http(co::coroutine_t *self, intptr_t data);

void HttpServer::process_input(HttpConnection &conn,
                               ListByteBuffer *)
{
    co::coroutine_ptr f = co::create(process_http);
    arg_t arg = std::make_tuple(&conn, _reader, _event.get());
    co::resume(f, (intptr_t)&arg);
}

struct ConnectionGuard
{
    ConnectionGuard(HttpConnection& conn)
        : _conn(conn) {}
    ~ConnectionGuard() { _conn.close(); }
    HttpConnection &_conn;
};

static
intptr_t process_http(co::coroutine_t *self, intptr_t data)
{
    const arg_t &arg = *(arg_t*)data;

    HttpConnection *pconn;
    GfsReader *reader;
    co::Event *event;
    std::tie(pconn, reader, event) = arg;
    const uint64_t id = (uint64_t)self;

    ConnectionGuard conn_guard(*pconn);


    // 1. parse the request
    const std::size_t blocksize = 1*1024*1024;
    std::string filename = "/tmp/test_file";
    fs::file_status status;
    fs::stat(status, filename.c_str());
    std::size_t length = fs::get_size(status);
    uint64_t offset = 0;

    // 2. output header
    HttpReply &reply = pconn->get_reply();
    reply.set_reply_content_length(length);
    pconn->out_put(0, 0, HTTP::OK);
    std::size_t buflen = (unsigned int)reply.body_buff_length();
    bool empty = reply.body_buff_empty();
    printf("header sent, buflen=%zd, empty=%d\n",
	   buflen, (int)empty);

    // 3. read and write data
    std::size_t remain = length;
    while(remain > 0)
    {
        std::size_t nread = (std::min)(blocksize, remain);
        
        GfsReader::dataarray_t *data = reader->read(
            id, self, 30,
            filename, nread, offset);

	std::size_t buflen = (unsigned int)reply.body_buff_length();
	bool empty = reply.body_buff_empty();
	printf("data read, buflen=%zd, empty=%d\n",
	   buflen, (int)empty);

        if(data == NULL)
        {
            // timeout
	  printf("empty data, nread=%zd, remain=%zd\n",
		 nread, remain);
            return 0;
        }

        if(data->size() != nread)
        {
            // exception!
	  printf("read error, nread=%zd, remain=%zd, "
		 "datasize=%zd\n",
		 nread, remain, data->size());
            return 0;
        }

        remain -= nread;
        offset += data->size();

        // send data in smaller block
        const std::size_t send_block_size = 4*1024;
        char *pdata = &(*data)[0];
        char *pend = pdata + nread;
        while(pdata != pend)
        {
            const std::size_t left_size = pend - pdata;
            std::size_t nsend = (std::min)(send_block_size,
					   left_size);
            pconn->out_put(pdata, nsend, HTTP::APPEND_CONTENT);
            pdata += nsend;
	    //???pconn->flush();
	    
            while(!reply.body_buff_empty())
	      event->usleep(self, 100);

	    std::size_t buflen = (unsigned int)reply.body_buff_length();
	    bool empty = reply.body_buff_empty();
	    printf("block sent, nsend=%zd, buflen=%zd, empty=%d\n",
		   nsend, buflen, (int)empty);
        }


	reader->release_data_array(data);
	printf("block sent, nread=%zd, remain=%zd\n",
		 nread, remain);
    }

    // bool e=reply.body_buff_empty();
    // while(!e)
    //   {
	event->sleep(self, 10);
      // 	e=reply.body_buff_empty();
      // }
    
    printf("end, emain=%zd\n",
	   remain);
    return 0;
}
    
