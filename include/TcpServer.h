#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#endif // TCPSERVER_HPP

#include "EventLoop.h"
#include "ThreadPool.h"

class TcpServer
{
public:
    // 初始化
    TcpServer(unsigned short port, int threadNum);
    // 析构函数
    ~TcpServer();

    // 初始化监听
    void setListen();
    // 启动服务器
    void run();

    static int acceptConnection(void* arg);

private:
    int m_threadNum;
    EventLoop *m_mainLoop;
    ThreadPool *m_threadPool;

    int m_lfd;
    unsigned short m_port;
};
