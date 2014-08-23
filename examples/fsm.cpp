#include <string>
#include <queue>
#include <thread>
#include <iostream>

#include <pthread.h>

#include <coroutine/all.hpp>
namespace co = coroutine;

struct event_base *evbase = event_base_new();

std::string mysql_execute(co::self_t self,
                          std::string sql,
                          long timeout_sec);

intptr_t process_mysql_req(co::self_t self,
                           intptr_t data)
{
    // 做个本地拷贝，不要持有外部栈上的变量
    std::string sql = *(std::string*)data;
    int timeout_sec = 3;
    std::string result;

    std::cout << "mysql_execute " << sql << std::endl;
    
    // 这个调用看起来很像堵塞调用，让整个函数处理流程清晰直观。
    result = mysql_execute(self, sql, timeout_sec);

    std::cout << "mysql_execute done " << result << std::endl;

    // do others
    // ...

    return 0;
}


// ==================模拟Mysql操作===============

struct MysqlReq
{
    co::self_t source;
    std::string sql;
};

struct MysqlResp
{
    co::self_t target;
    std::string result;
};

std::queue<MysqlReq> mysql_thread_inqueue;
std::queue<MysqlResp> mysql_thread_outqueue;
pthread_mutex_t mysql_thread_inqueue_mutex;
pthread_mutex_t mysql_thread_outqueue_mutex;
co::Dispatcher mysql_dispatcher;

std::string mysql_execute(std::string sql);

bool mysql_thread_stop = false;
void mysql_thread()
{
    while(! mysql_thread_stop)
    {
        bool empty = false;
        while(true)
        {
            pthread_mutex_lock(&mysql_thread_inqueue_mutex);
            empty = mysql_thread_inqueue.empty();
            pthread_mutex_unlock(&mysql_thread_inqueue_mutex);

            if(! empty) break;
            
            usleep(20);
        }

        pthread_mutex_lock(&mysql_thread_inqueue_mutex);
        MysqlReq req = mysql_thread_inqueue.front();
        mysql_thread_inqueue.pop();
        pthread_mutex_unlock(&mysql_thread_inqueue_mutex);

        std::string result = mysql_execute(req.sql);

        MysqlResp resp;
        resp.target = req.source;
        resp.result = result;

        pthread_mutex_lock(&mysql_thread_outqueue_mutex);
        mysql_thread_outqueue.push(resp);
        pthread_mutex_unlock(&mysql_thread_outqueue_mutex);
    }
}

std::string mysql_execute(std::string sql)
{
    // 模拟mysql操作
    sql += "Hello Coroutine!";
    return sql;
}

// ====================================================

std::string mysql_execute(co::self_t self,
                          std::string sql,
                          long timeout_sec)
{
    MysqlReq req;
    req.source = self;
    req.sql = sql;

    pthread_mutex_lock(&mysql_thread_inqueue_mutex);
    mysql_thread_inqueue.push(req);
    pthread_mutex_unlock(&mysql_thread_inqueue_mutex);

    intptr_t retval;
    bool ok;
    std::tie(retval, ok) =
        mysql_dispatcher.wait(self, timeout_sec);
    assert(ok);

    std::string *result = (std::string*)retval;
    return *result;
}

int mysql_thread_poll(int max_process_once = 64)
{
    int i = 0;
    while(i<max_process_once)
    {
        bool empty = false;
        pthread_mutex_lock(&mysql_thread_outqueue_mutex);
        empty = mysql_thread_outqueue.empty();
        pthread_mutex_unlock(&mysql_thread_outqueue_mutex);
        
        if(empty) break;
        ++i;

        pthread_mutex_lock(&mysql_thread_outqueue_mutex);
        MysqlResp resp = mysql_thread_outqueue.front();
        mysql_thread_outqueue.pop();
        pthread_mutex_unlock(&mysql_thread_outqueue_mutex);
        
        intptr_t data = (intptr_t)&resp.result;
        int n = mysql_dispatcher.notify(resp.target, data);
        assert(n == 1);
    }
    return i;
}


intptr_t process_mysql_req(co::self_t self,
                           intptr_t data);

// 模拟收到用户请求的处理
void process_input(std::string request)
{
    bool is_mysql_request = true;
    if(is_mysql_request)
    {
        co::coroutine_t f =
            co::create(process_mysql_req, evbase);
        co::resume(f, (intptr_t)&request);
    }
    // others
}

void network_poll()
{
    std::string request("Hello World!");
    process_input(request);
}

int main(int argc, char **argv)
{
    pthread_mutex_init(&mysql_thread_inqueue_mutex, NULL);
    pthread_mutex_init(&mysql_thread_outqueue_mutex, NULL);

    // start mysql thread
    std::thread mysql(mysql_thread);

    while(true)
    {
        mysql_thread_poll();
        network_poll();         // 类似reactor->poll()
    }

    mysql_thread_stop = true;
    mysql.join();

    return 0;
}

