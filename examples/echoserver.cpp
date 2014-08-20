#include <coroutine/all.hpp>
namespace co = coroutine;

intptr_t echo_server(co::self_t self, intptr_t data)
{
    evutil_socket_t fd;
    struct sockaddr *paddr;
    ev_socklen_t addr_len;
    std::tie(fd, paddr, addr_len) =
        *(co::acceptor_param_t*)data;
    
    if(fd < 0)
    {
        return -1;
    }

    co::fd_close_guard guard(fd);
    
    struct sockaddr_storage addr;
    memcpy(&addr, paddr, addr_len);

    std::cout << "new connection " << fd << std::endl;
    
    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = 3;
    
    const std::size_t buf_len = 1024;
    char buf[buf_len];
    ssize_t nread;
    ssize_t nwrite;
    while(true)
    {
        nread = read(self, &tv, fd, buf, buf_len);
        if(nread < 0)               // read error
        {
            int e = evutil_socket_geterror(fd);
            std::cerr << "read error " << e << std::endl;
            break;
        }

        if(nread == 0)          // eof
        {
            std::cout << "closed or timeout " << fd
                      << std::endl;
            break;
        }

        buf[nread] = '\0';
        std::cout << "read " << std::string(buf) << std::endl;

        nwrite = writen(self, &tv, fd, buf, nread);
        if(nwrite != nread)     // write error
        {
            break;
        }
        std::cout << "write " << std::string(buf) << std::endl;
    }

    return 0;
}

intptr_t acceptor(co::self_t self, intptr_t data)
{
    const char *ip_port = (const char *)data;
    struct event_base *evbase = get_event_base(self);

    evconnlistener *listener = listen(self, ip_port);
    if(listener == NULL)
    {
        std::cout << "listen fail on " << ip_port << std::endl;
        return -1;
    }
    co::listener_guard guard(listener);

    struct sockaddr remote_addr;
    ev_socklen_t addr_len = sizeof(struct sockaddr);
    evutil_socket_t new_fd;
    co::acceptor_param_t fd_info;
    while((new_fd = accept(self, &remote_addr,
                           &addr_len)) != -1)
    {
        co::coroutine_t echof =
            co::create(echo_server, evbase);
        fd_info = std::make_tuple(new_fd,
                                  &remote_addr,
                                  addr_len);
        co::resume(echof, &fd_info);
    }

    return 0;
}

int main(int argc, char** argv)
{
    const char *ip_port = "127.0.0.1:8080";

    if(argc > 1)
        ip_port = argv[1];

    struct event_base *evbase = event_base_new();

    co::coroutine_t f = co::create(acceptor, evbase);
    co::resume(f, (intptr_t)ip_port);

    event_base_dispatch(evbase);

    event_base_free(evbase);
    
    return 0;
}

