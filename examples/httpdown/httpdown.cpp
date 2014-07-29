
#include "http_server.hpp"
#include "gfs_reader.hpp"

int main()
{
    struct event_base *base = event_base_new();
    GfsReader *reader = new GfsReader(base);
    HttpServer *server = new HttpServer(8080, reader);

    while(true)
    {
        event_base_loop(base, EVLOOP_NONBLOCK);
        reader->poll();
        server->poll();
    }
    
}

