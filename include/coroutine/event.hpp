// -*-mode:c++; coding:utf-8-*-
#ifndef _COROUTINE_EVENT_HPP_
#define _COROUTINE_EVENT_HPP_

#include "core.hpp"

#include <event2/event.h>
#include <event2/listener.h>

namespace coroutine
{

    void poll_event_base(struct event_base *evbase);

    void sleep(self_t c, long sec, long usec);

    inline
    void sleep(self_t c, long sec)
    {
        sleep(c, sec, 0);
    }

    inline
    void usleep(self_t c, long usec)
    {
        sleep(c, 0, usec);
    }


    // 
    // 下面一组接口是对fd操作的
    // 

    struct fd_close_guard
    {
        evutil_socket_t _fd;
        explicit
        fd_close_guard(evutil_socket_t fd)
            : _fd(fd) {}
        ~fd_close_guard()
            { if(_fd >= 0) evutil_closesocket(_fd); }
        void release() { _fd = -1; }

    private:
        fd_close_guard(const fd_close_guard &);
        fd_close_guard &operator=(const fd_close_guard &);
    };

    struct listener_guard
    {
        evconnlistener *_l;
        explicit
        listener_guard(evconnlistener *l)
            : _l(l) {}
        ~listener_guard()
            { if(_l) evconnlistener_free(_l); }
        bool empty() const { return _l == NULL; }

    private:
        listener_guard(const listener_guard &);
        listener_guard &operator=(const listener_guard &);
    };

    
    typedef std::tuple<evutil_socket_t,
                       struct sockaddr *,
                       ev_socklen_t> acceptor_param_t;

    evconnlistener *listen(self_t acceptor,
                           const char *ip_port);

    evutil_socket_t accept(self_t acceptor,
                           struct sockaddr *addr,
                           ev_socklen_t *addr_len);

    evutil_socket_t connect(self_t connector,
                            const char *ip_port,
                            const struct timeval *tv = NULL);

    ssize_t read(self_t reader,
                 const struct timeval *tv,
                 evutil_socket_t fd,
                 char *buf,
                 std::size_t buf_len);

    ssize_t readn(self_t reader,
                  const struct timeval *tv,
                  evutil_socket_t fd,
                  char *buf,
                  std::size_t buf_len);

    ssize_t write(self_t writer,
                  const struct timeval *tv,
                  evutil_socket_t fd,
                  const char *data,
                  std::size_t data_len);

    ssize_t writen(self_t writer,
                   const struct timeval *tv,
                   evutil_socket_t fd,
                   const char *data,
                   std::size_t data_len);

}

#endif  // _COROUTINE_EVENT_HPP_


