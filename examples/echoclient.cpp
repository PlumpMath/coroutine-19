#include <coroutine/all.hpp>
namespace co = coroutine;

#include <iostream>

#include <signal.h>

intptr_t echo_client(co::self_t self, intptr_t data)
{
    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = 3;

    const char *ip_port = (const char *)data;
    evutil_socket_t fd = co::connect(self, ip_port, &tv);
    if(fd < 0)
    {
        std::cout << "connect fail " << errno << std::endl;
        return -1;
    }

    co::fd_close_guard guard(fd);

    std::cout << "connected " << fd << std::endl;

    char str[] = "hello server!";
    const size_t str_len = sizeof(str);
    char buf[1024];
    size_t buf_len = sizeof(buf);
    for(int i=0; i<1; ++i)
    {
        ssize_t wbytes =
            writen(self, &tv, fd, str, str_len);
        std::cout << "writen wbytes " << wbytes << std::endl;
        if(wbytes != (ssize_t)str_len)
        {
            int e = evutil_socket_geterror(fd);
            std::cerr << "write error " << e << std::endl;
            break;
        }
        std::cout << "write " << std::string(str) << std::endl;
        
        ssize_t rbytes =
            read(self, &tv, fd, buf, buf_len);
        if(rbytes < 0)
        {
            int e = evutil_socket_geterror(fd);
            std::cerr << "read error " << e << std::endl;
            break;
        }
        if(rbytes == 0)
        {
            std::cout << "closed or timeout "
                      << fd << std::endl;
            break;
        }
        
        buf[rbytes] = '\0';
        std::cout << "recv " << std::string(buf) << std::endl;
    }

    std::cout << "closed " << fd << std::endl;
    return 0;
}

void ignore_pipe(void)
{
    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sig, 0);
}

int main(int argc, char **argv)
{
    ignore_pipe();

    const char *ip_port = "127.0.0.1:8080";
    int nloop = 10;

    if(argc > 1)
        ip_port = argv[1];
    if(argc > 2)
        nloop = atoi(argv[2]);

    struct event_base *evbase = event_base_new();

    for(int i=0; i<nloop; ++i)
    {
        co::coroutine_t connector =
            co::create(echo_client, evbase);
        co::resume(connector, (intptr_t)ip_port);
    }

    event_base_dispatch(evbase);

    event_base_free(evbase);

    return 0;
}
