#include <coroutine/event.hpp>

#include <memory>
#include <string.h>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <event2/event_struct.h>
#include <event2/util.h>

namespace coroutine
{

    void poll_event_base(struct event_base *evbase)
    {
        int flags = EVLOOP_NONBLOCK;
        int rv = event_base_loop(evbase, flags);
        assert(rv != -1);
        (void)rv;
    }

    static
    void timeout_callback(evutil_socket_t fd,
                          short event,
                          void *arg)
    {
        self_t c = (self_t)arg;
        resume(c);
    }

    void sleep(self_t c, long sec, long usec)
    {
        struct event_base *evbase = get_event_base(c);
        struct event timeout;
        struct timeval tv;

        evtimer_assign(&timeout, evbase,
                       timeout_callback, (void*)c);
                       
        evutil_timerclear(&tv);
        tv.tv_sec = sec;
        tv.tv_usec = usec;
        event_add(&timeout, &tv);

        coroutine_t hold(c);  // prevent destroy
        yield(c);
    }

    // recognized formats are:
    // [ipv6]:port
    // ipv6
    // [ipv6]
    // ipv4:port
    // ipv4
    // port (as 127.0.0.1:port)
    static
    int parse_ip_port(const char *ip_port,
                      struct sockaddr *addr,
                      ev_socklen_t *addr_len)
    {
        int addr_len_ = *addr_len;
        int parse = evutil_parse_sockaddr_port(
            ip_port, addr, &addr_len_);
        *addr_len = addr_len_;
            
        if(parse < 0)           // fail, try as port
        {
            int p = atoi(ip_port);
            if(p < 1 || p > 65535)
                return -1;
     
            struct sockaddr_in *sin =
                (struct sockaddr_in*)addr;
            sin->sin_port = htons(p);
            sin->sin_addr.s_addr = htonl(0x7f000001);
            sin->sin_family = AF_INET;
            *addr_len = sizeof(struct sockaddr_in);
        }

        return 0;
    }

    static
    void accept_cb(struct evconnlistener *listener,
                   evutil_socket_t fd,
                   struct sockaddr *addr,
                   int addr_len,
                   void *p)
    {
        self_t acceptor = (self_t)p;
        acceptor_param_t param = 
            std::make_tuple(fd, addr, (ev_socklen_t)addr_len);
        resume(acceptor, &param);
    }

    evconnlistener *listen(self_t acceptor, const char *ip_port)
    {
        struct sockaddr_storage listen_on_addr;
        memset(&listen_on_addr, 0, sizeof(listen_on_addr));
        ev_socklen_t socklen = sizeof(listen_on_addr);
        int parse = parse_ip_port(
            ip_port,
            (struct sockaddr*)&listen_on_addr,
            &socklen);
        if(parse < 0)
            return NULL;

        int backlog = -1;       // reasonable default value
        evconnlistener *listener = evconnlistener_new_bind(
            get_event_base(acceptor),
            accept_cb, acceptor,
            LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC |
            LEV_OPT_REUSEABLE,
            backlog,
            (struct sockaddr*)&listen_on_addr, socklen);

        return listener;
    }

    evutil_socket_t accept(self_t acceptor,
                           struct sockaddr *addr,
                           ev_socklen_t *addr_len)
    {
        evutil_socket_t fd;
        struct sockaddr *ret_addr;
        ev_socklen_t ret_addr_len;
        
        coroutine_t hold(acceptor);
        std::tie(fd, ret_addr, ret_addr_len) =
            *(acceptor_param_t*)yield(acceptor);
        
        memcpy(addr, ret_addr, ret_addr_len);
        *addr_len = ret_addr_len;
        
        return fd;
    }

    static
    void connect_cb(evutil_socket_t fd, short event, void *arg)
    {
        self_t connector = (self_t)arg;
        resume(connector, event);
    }

    evutil_socket_t connect(self_t connector,
                            const char *ip_port,
                            const struct timeval *tv)
    {
        struct sockaddr_storage connect_to_addr;
        memset(&connect_to_addr, 0, sizeof(connect_to_addr));
        ev_socklen_t addr_len = sizeof(connect_to_addr);

        int parse = parse_ip_port(
            ip_port,
            (struct sockaddr *)&connect_to_addr,
            &addr_len);
        if(parse < 0)
        {
            return -1;
        }

        evutil_socket_t fd =
            socket(connect_to_addr.ss_family, SOCK_STREAM, 0);
        if(fd < 0)
        {
            return -1;
        }

        fd_close_guard fd_guard(fd);
        if(evutil_make_socket_nonblocking(fd) < 0)
        {
            return -1;
        }

        int rv =
            connect(fd,
                    (const struct sockaddr *)&connect_to_addr,
                    addr_len);
        if(rv == 0)
        {
            fd_guard.release();
            return fd;
        }

        int e = evutil_socket_geterror(fd);

        int err = 0;
        socklen_t err_len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
        std::cout << "err " << err << std::endl;

        // true iff e is an error that means an connect can be retried.
        if(e == EINTR || e == EINPROGRESS)
        {
            // register event and wait
            struct event connect_event;
            short events = EV_WRITE;
            if(tv && evutil_timerisset(tv))
            {
                events |= EV_TIMEOUT;
            }
            event_assign(
                &connect_event,
                get_event_base(connector),
                fd, events,
                connect_cb,
                connector);
            if(tv && evutil_timerisset(tv))
            {
                event_add(&connect_event, tv);
            }
            else
            {
                event_add(&connect_event, NULL);
            }

            std::cout << "wait for connect events"
                      << std::endl;
            coroutine_t hold(connector);
            short got_event = yield(connector);
            std::cout << "got connect events " << got_event
                      << std::endl;

            if(got_event & EV_TIMEOUT)
            {
                return -1;
            }

            int err = 0;
            socklen_t err_len = sizeof(err);
            int rv_getsockopt =
                getsockopt(fd, SOL_SOCKET, SO_ERROR,
                           &err, &err_len);
            if(rv_getsockopt < 0)
            {
                return -1;
            }
            std::cout << "err " << err
                      << ", errno " << errno << std::endl;
            if(err == ECONNREFUSED)
            {
                return -1;      // connect refused
            }

            fd_guard.release(); // connect success
            return fd;
        }

        // // refused
        // if (EVUTIL_ERR_CONNECT_REFUSED(e))
        // {
        //     return -1;
        // }

        return -1;              // connect fail
    }

    static
    void read_cb(evutil_socket_t fd, short event, void *arg)
    {
        self_t reader = (self_t)arg;
        resume(reader, event);
    }

    ssize_t read(self_t reader,
                 const struct timeval *tv,
                 evutil_socket_t fd,
                 char *buf,
                 std::size_t buf_len)
    {
        // register read event
        struct event read_event;
        short events = EV_READ;
        if(tv && evutil_timerisset(tv))
        {
            events |= EV_TIMEOUT;
        }
        event_assign(&read_event,
                     get_event_base(reader),
                     fd, events,
                     read_cb, reader);
        if(tv && evutil_timerisset(tv))
        {
            event_add(&read_event, tv);
        }
        else
        {
            event_add(&read_event, NULL);
        }

        // wait for event
        coroutine_t hold(reader);
        short got_event = yield(reader);

        ssize_t bytes = 0;
        if(got_event & EV_TIMEOUT)
        {
            bytes = 0;
        }
        else
        {
            bytes = ::read(fd, buf, buf_len);
        }
        
        return bytes;
    }

    ssize_t readn(self_t reader,
                  const struct timeval *tv,
                  evutil_socket_t fd,
                  char *buf,
                  std::size_t buf_len)
    {
        // register read event
        struct event read_event;
        short events = EV_READ|EV_PERSIST;
        if(tv && evutil_timerisset(tv))
        {
            events |= EV_TIMEOUT;
        }
        event_assign(&read_event,
                     get_event_base(reader),
                     fd, events,
                     read_cb, reader);
        if(tv && evutil_timerisset(tv))
        {
            event_add(&read_event, tv);
        }
        else
        {
            event_add(&read_event, NULL);
        }

        ssize_t total_bytes = 0;
        char *pbuf = buf;
        std::size_t left_len = buf_len;
        while(true)
        {
            // wait for event
            coroutine_t hold(reader);
            short got_event = yield(reader);

            if(got_event & EV_TIMEOUT)
            {
                break;
            }
            
            ssize_t bytes = ::read(fd, pbuf, left_len);
            if(bytes >= 0)
            {
                total_bytes += bytes;
                pbuf += bytes;
                left_len -= bytes;

                if(left_len == 0)
                    break;
            }
            else
            {
                if(total_bytes == 0)
                    total_bytes = -1;
                
                break;
            }
        }

        event_del(&read_event);

        return total_bytes;
    }

    static
    void write_cb(evutil_socket_t fd, short event, void *arg)
    {
        self_t writer = (self_t)arg;
        resume(writer, event);
    }

    ssize_t write(self_t writer,
                  const struct timeval *tv,
                  evutil_socket_t fd,
                  const char *data,
                  std::size_t data_len)
    {
        struct event write_event;
        short events = EV_WRITE;
        if(tv && evutil_timerisset(tv))
        {
            events |= EV_TIMEOUT;
        }
        event_assign(&write_event,
                     get_event_base(writer),
                     fd, events,
                     write_cb, writer);
        if(tv && evutil_timerisset(tv))
        {
            event_add(&write_event, tv);
        }
        else
        {
            event_add(&write_event, NULL);
        }

        std::cout << "write wait " << fd << std::endl;
        // wait for event
        coroutine_t hold(writer);
        short got_event = yield(writer);

        ssize_t bytes = 0;
        if(got_event & EV_TIMEOUT)
        {
            bytes = 0;
        }
        else
        {
            bytes = ::write(fd, data, data_len);
        }

        return bytes;
    }

    ssize_t writen(self_t writer,
                   const struct timeval *tv,
                   evutil_socket_t fd,
                   const char *data,
                   std::size_t data_len)
    {
        struct event write_event;
        short events = EV_WRITE|EV_PERSIST;
        if(tv && evutil_timerisset(tv))
        {
            events |= EV_TIMEOUT;
        }
        event_assign(&write_event,
                     get_event_base(writer),
                     fd, events,
                     write_cb, writer);
        if(tv && evutil_timerisset(tv))
        {
            int rv = event_add(&write_event, tv);
            assert(rv == 0);
        }
        else
        {
            int rv = event_add(&write_event, NULL);
            assert(rv == 0);
        }

        ssize_t total_bytes = 0;
        const char *pdata = data;
        std::size_t left_len = data_len;
        while(true)
        {
            std::cout << "writen wait " << fd << std::endl;

            // wait for event
            coroutine_t hold(writer);
            short got_event = yield(writer);

            if(got_event & EV_TIMEOUT)
            {
                break;
            }

            std::cout << "writen real write " << left_len << std::endl;
            ssize_t bytes = ::write(fd, pdata, left_len);
            std::cout << "writen bytes " << bytes << std::endl;
            if(bytes >= 0)
            {
                total_bytes += bytes;
                pdata += bytes;
                left_len -= bytes;

                if(left_len == 0)
                    break;
            }
            else
            {
                if(total_bytes == 0)
                    total_bytes = -1;
                
                break;
            }
        }

        event_del(&write_event);

        return total_bytes;
    }

}
