#include "http_server.hpp"

#include <tuple>
#include <algorithm>
#include <sys/time.h>

#include <gce/gfe/http_reactor.h>
#include <appframe/fixed_size_allocator.h>

#include <coroutine-cpp/event.hpp>

//#include "gfs.hpp"
#include "localfs.hpp"
namespace fs = localfs;

#include "gfs_reader.hpp"

typedef std::tuple<long,		// connid
                   HttpReactor*,
                   GfsReader*,
                   FixedSizeAllocator*> arg_t;

HttpServer::HttpServer(int port,
                       GfsReader *reader,
                       struct event_base *evbase)
    : _reader(reader)
    , _evbase(evbase)
{
    const int page_size = 4*1024;
    const int mem_size = 40*1024*1024;
    const char *server_name = "nginx/1.2.7";

    _allocator = allocator_ptr(new FixedSizeAllocator(
                                   page_size, page_size,
                                   mem_size));
    if(_allocator->init() < 0)
        throw std::runtime_error("init allocator fail");

    const long select_timeout = 0; // cause epoll_wait return immediately
    _reactor = reactor_ptr(new HttpReactor(_allocator.get(),
                                           this, port,
                                           HttpReactor::DEFAULT_PORT_BAK, 
                                           HttpReactor::DEFAULT_MAX_CONNECTIONS,
                                           HttpReactor::DEFAULT_ACCECPT_ONCE,
                                           HttpReactor::DEFAULT_BACKLOG,
                                           10,
                                           10,
                                           30,
                                           NULL,
                                           select_timeout));
    if(_reactor->init(server_name) < 0)
        throw std::runtime_error("init reactor fail");
}

HttpServer::~HttpServer()
{
}

int HttpServer::poll()
{
    return _reactor->poll();
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
    co::coroutine_ptr f = co::create(process_http, _evbase);
    arg_t arg = std::make_tuple(conn.id(),
                                _reactor.get(),
                                _reader,
                                _allocator.get());
    co::resume(f, (intptr_t)&arg);
}

struct DataGuard
{
    DataGuard(char *data, GfsReader *reader)
        : _data(data), _reader(reader) {}
    ~DataGuard() { _reader->release_data_array(_data); }
    char *_data;
    GfsReader *_reader;
};

#define CONN_CHECK_OR_RETURN(id) do { if(reactor->connection(id)==NULL) return 0; } while(false)

static
intptr_t process_http(co::coroutine_t *self, intptr_t data)
{
    const arg_t &arg = *(arg_t*)data;

    long connid;
    HttpReactor *reactor;
    GfsReader *reader;
    FixedSizeAllocator *allocator;
    std::tie(connid, reactor, reader, allocator) = arg;
    
    const uint64_t id = connid;
    CONN_CHECK_OR_RETURN(connid);
    HttpConnection *pconn = reactor->connection(id);

    // 1. parse the request
    const std::size_t blocksize = 1*1024*1024;
    std::string filename = "/tmp/test_file";
    fs::file_status status;
    fs::stat(status, filename.c_str());
    std::size_t length = fs::get_size(status);
    uint64_t offset = 0;


    // speed control
    int speed_per_second = 10 * 1024 * 1024; // N MB/s
    const std::size_t send_block_size = 4*1024;
    // bp<us>= 1000*1000 <us>p<s> / (n Bps / n B)
    long n_usec = 1000*1000/(speed_per_second / send_block_size);

    // 2. output header
    CONN_CHECK_OR_RETURN(connid);
    HttpReply &reply = pconn->get_reply();
    reply.set_reply_content_length(length);
    pconn->out_put(0, 0, HTTP::OK);
    std::size_t buflen = (unsigned int)reply.body_buff_length();
    bool empty = reply.body_buff_empty();
    printf("conn %ld header sent, buflen=%zd, empty=%d\n",
           connid, buflen, (int)empty);

    // 3. read and write data
    std::size_t remain = length;
    while(remain > 0)
    {
        std::size_t nread = (std::min)(blocksize, remain);
        
        char *data = reader->read(
            id, self, 30,
            filename, nread, offset);

        DataGuard guard(data, reader);

        std::size_t buflen = (unsigned int)reply.body_buff_length();
        bool empty = reply.body_buff_empty();
        printf("conn %ld data read, buflen=%zd, empty=%d\n",connid,
               buflen, (int)empty);

        if(data == NULL)
        {
            // timeout
            printf("conn %ld empty data, nread=%zd, remain=%zd\n",
                   connid, nread, remain);
            CONN_CHECK_OR_RETURN(connid);
            pconn->close();
            return 0;
        }

        remain -= nread;
        offset += nread;

        // send data in smaller block
        char *pdata = data;
        char *pend = pdata + nread;
        while(pdata != pend)
        {
            const std::size_t left_size = pend - pdata;
            std::size_t nsend = (std::min)(send_block_size,
                                           left_size);


            std::size_t free_blocks = allocator->free_blocks();
            if(free_blocks < 10)
            {
                printf("WARN allocator is used out,"
                       " free_blocks:%ld\n",free_blocks);
            }

            CONN_CHECK_OR_RETURN(connid);
            pconn->out_put(pdata, nsend, HTTP::APPEND_CONTENT);
            printf("conn %ld reply body buf empty:%d len:%d\n",
                   connid,
                   reply.body_buff_empty(),
                   reply.body_buff_length());
            pdata += nsend;

            printf("conn %ld reply_write=%ld, conn_write=%ld\n",
                   connid,
                   reply.cur_emit_data_num(),
                   pconn->written_bytes());

            // speed control
            co::usleep(self, n_usec);

            // cache control, 防止接收慢还一直发
            bool show = false;
            while(true) 
            {
                CONN_CHECK_OR_RETURN(connid);
                if(reply.body_buff_length() < (long)send_block_size * 8)
                    break;
                if(!show) printf("slow down, buf too big:%ld",
                                 (long)reply.body_buff_length());
                co::usleep(self, n_usec);
            }
        }

        printf("conn %ld block sent, nread=%zd, remain=%zd\n",
               connid, nread, remain);
    }

    CONN_CHECK_OR_RETURN(connid);
    pconn->close();
    printf("conn %ld end, remain=%zd\n", connid, remain);
    return 0;
}
    
