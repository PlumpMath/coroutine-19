#include <http_server.hpp>

#include <tuple>

#include <gce/gfe/http_reactor.h>
#include <appframe/fixed_size_allocator.h>

#include <gfs_reader.hpp>

typedef std::tuple<HttpConnection*,
                   GfsReader*> arg_t;

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
    
    _reactor = reactor_ptr(new HttpReactor(_allocator,
                                           this, port));
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
}

void HttpServer::remove(HttpConnection &conn)
{
}

intptr_t process_http(co::coroutine_t *self, intptr_t data);

void HttpServer::process_input(HttpConnection &conn,
                               ListByteBuffer *)
{
    co::coroutine_ptr f = co::create(http_server);
    arg_t arg = std::make_tuple(&conn, _reader);
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
    std::tie(pconn, reader) = arg;

    ConnectionGuard conn_guard(*pconn);


    // 1. parse the request

    // 2. output header
    pconn->output(0, 0, HTTP::OK);

    // 3. read and write data
    const std::size_t blocksize = 1*1024*1024;
    std::string filename = "/tmp/test_file";
    fs::file_status status;
    fs::stat(status, filename);
    std::size_t length = fs::get_size(status);
    uint64_t offset = 0;
    std::size_t remain = length;
    while(remain > 0)
    {
        std::size_t nread = (std::min)(blocksize, remain);
        
        uintptr_t filedata = gfs_thread.read_file(
            pconn->connid(), self, 30,
            filename, nread, offset);
        std::vector<char> *data = (std::vector<char> *)filedata;

        if(data == NULL)
        {
            // timeout
            return;
        }

        if(data->size() != nread)
        {
            // exception!
            return;
        }

        remain -= nread;
        offset += data->size();

        // send data in smaller block
        const std::size_t send_block_size = 4*1024;
        char *pdata = &(*data)[0];
        char *pend = pdata + nread;
        while(pdata != pend)
        {
            std::size_t nsend = (std::min)(send_block_size,
                                           pend - pdata);
            pconn->output(pdata, nsend, HTTP::APPEND_CONTENT);
            pdata += nsend;
            co::sleep(self, 5);
        }
    }

}
    
