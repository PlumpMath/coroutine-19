#include <stdio.h>

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include <coroutine-cpp/event.hpp>

#include "http_server.hpp"
#include "gfs_reader.hpp"

/*
static
long operator-(const timeval &tv1,
               const timeval &tv2)
{
    long u1 = tv1.tv_sec;
    u1 *= 1000 * 1000;
    u1 += tv1.tv_usec;

    long u2 = tv2.tv_sec;
    u2 *= 1000 * 1000;
    u2 += tv2.tv_usec;

    return u1 - u2;
}
*/

void ignore_pipe(void)
{
    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sig, 0);
}

int main()
{
    ignore_pipe();

    struct event_base *base = event_base_new();
    GfsReader *reader = new GfsReader();
    HttpServer *server = new HttpServer(8080, reader, base);

    //timeval tv1,tv2,tv3,tv4;
    while(true)
    {
        //gettimeofday(&tv1, NULL);
        co::poll_event_base(base);
		//gettimeofday(&tv2, NULL);
        reader->poll();
		//gettimeofday(&tv3, NULL);
        server->poll();
		//gettimeofday(&tv4, NULL);
        // printf("main loop, base:%lu, reader:%lu, server:%lu\n",
        //        (tv2-tv1), tv3-tv2, tv4-tv3);
		
    }

    
}

