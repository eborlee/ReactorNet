#include "../include/TcpConnection.h"
#include "../include/Buffer.h" // Add missing include directive for the "Buffer" class
#include "../include/Channel.h"
#include "../include/Log.h"

TcpConnection::TcpConnection(int fd, EventLoop *evLoop)
{
    m_evLoop = evLoop;
    m_readBuf = new Buffer(10240);
    m_writeBuf = new Buffer(10240);
    // http
    m_request = new HttpRequest();
    m_response = new HttpResponse();

    m_name = "Connection-" + std::to_string(fd);
    // processRead就是要处理客户端传过来的内容进行http协议解析，根据请求的内容组织响应。
    m_channel = new Channel(fd, FDEvent::ReadEvent, processRead,
                            processWrite, destroy,
                            this);
    evLoop->addTask(m_channel, ElemType::ADD);
    Debug("和客户端建立连接, threadName: %s, threadID: %s, connName: %s",
          evLoop->getThreadName().data(), evLoop->getThreadID(), m_name.data());
}

TcpConnection::~TcpConnection()
{

    if (m_readBuf && m_readBuf->getReadableSize() == 0 && m_writeBuf && m_writeBuf->getReadableSize() == 0)
    {
        delete m_readBuf;
        delete m_writeBuf;
        delete m_request;
        delete m_response;
        m_evLoop->destroyChannel(m_channel);
    }

    Debug("连接断开，释放资源，connName: %s", m_name.data());
}

int TcpConnection::processRead(void *arg)
{
    struct TcpConnection *conn = (struct TcpConnection *)arg;
    int socket = conn->m_channel->getSocket();
    // 接收数据
    int count = conn->m_readBuf->socketRead(socket);

    Debug("接收到的http请求数据: %s", conn->m_readBuf->getData());
    if (count > 0)
    {
        // 接收到了http请求，解析

#ifdef MSG_SEND_AUTO
        // 添加写事件
        conn->m_channel->writeEventEnable(true);
        conn->m_evLoop->addTask(conn->m_channel, MODIFY);
#endif
        bool flag = conn->m_request->parseHttpRequest(conn->m_readBuf,
                                                      conn->m_response,
                                                      conn->m_writeBuf,
                                                      socket);
        if (!flag)
        {
            // 解析失败， 回复一个简单的html
            const char *errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            conn->m_writeBuf->append(const_cast<char *>(errMsg));
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 断开连接（我的理解是这句话在这个函数中应该删除？不应该让读回调去关闭连接吧？ Yes
        conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);

#endif
    }

#ifndef MSG_SEND_AUTO
    conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
    return 0;
}

int TcpConnection::processWrite(void *arg)
{
    Debug("开始发送数据");
    struct TcpConnection *conn = (struct TcpConnection *)arg;
    // 发送数据
    int socket = conn->m_channel->getSocket();
    int count = conn->m_writeBuf->sendData(socket);

    if (count > 0)
    {
        // 判断buffer中数据有无完全发送？
        if (conn->m_writeBuf->getReadableSize() == 0)
        {
            // 删除写事件
            conn->m_channel->writeEventEnable(false);
            conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
            conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
        }
        // if(bufferReadableSize(conn->writeBuf) == 0){
        //     // 以下两行代码可加可不加，因为如果说处理完请求发回数据给客户端之后就断开连接的话。
        //     // 不再检测写事件 -- 修改channel中保存的事件
        //     writeEventEnable(conn->channel, false);
        //     // 修改dispatcher检测的集合 -- 添加任务节点
        //     eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
        //     // 删除这个节点
        //     eventLoopAddTask(conn->evLoop, conn->channel, DELETE);

        // }
    }
}

int TcpConnection::destroy(void *arg)
{
    struct TcpConnection *conn = (struct TcpConnection *)arg;
    if (conn != nullptr)
    {
        delete conn;
    }
    return 0;
}
