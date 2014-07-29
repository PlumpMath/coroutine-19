#include "http_server.hpp"
#include "gfs_reader.hpp"

int main()
{
    GfsReader *reader = new GfsReader();
    HttpServer *server = new HttpServer(8080, reader);

    while(true)
    {
        reader->poll();
        server->poll();
    }
    
}

