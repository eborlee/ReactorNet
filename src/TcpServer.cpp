#include "TcpServer.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

TcpServer::TcpServer(unsigned short port, int threadNum)
{
    m_port = port;
    m_mainLoop = new EventLoop;
    m_threadNum = threadNum;
    m_threadPool = new ThreadPool(m_mainLoop, m_threadNum);
    setListen();
}

TcpServer::~TcpServer()
{
    delete m_mainLoop;
    delete m_threadPool;
}

void TcpServer::setListen()
{
    // 创建监听的fd, 流式协议，TCP协议 IPV4
    // Include the header file that defines AF_INET

    m_lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_lfd == -1)
    {
        perror("socket");
        return;
    }
    // 设置端口复用, 使得
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
    }
    // 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;     // IPV4
    addr.sin_port = htons(m_port); // 小端转大端 何意？什么字节序
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_lfd, (struct sockaddr *)&addr, sizeof addr);
    if (ret == -1)
    {
        perror("bind");
    }
    // 设置监听
    ret = listen(m_lfd, 128);
    if (ret == -1)
    {
        perror("listen");
    }
}

void TcpServer::run()
{
    // Debug("服务器程序启动了")
    // 启动线程池
    m_threadPool->run();

    // 此处声明了lfd被激活的事件类型和回调函数及其参数
    Channel *channel = new Channel(m_lfd, FDEvent::ReadEvent,
                                   TcpServer::acceptConnection, nullptr, nullptr, this);
    // 添加监听描述符的检测任务
    m_mainLoop->addTask(channel, ElemType::ADD);
    // 此时用于监听有无新连接请求的lfd已经被添加到主线程evloop的epoll检测集合里（这个集合也只有这一个待检测的文件描述符

    // 启动反应堆模型
    m_mainLoop->run();
}

int TcpServer::acceptConnection(void *arg)
{
    struct TcpServer *server = (struct TcpServer *)arg;
    // 和客户端建立连接
    int cfd = accept(server->m_lfd, NULL, NULL);
    // 取一个子线程的反应堆实例处理后续通信事件
    
    EventLoop *evLoop = server->m_threadPool->takeWorkerEventLoop();
    // 将cfd放到TcpConn中处理
    tcoConnectionInit(cfd, evLoop);
    return 0;
}
