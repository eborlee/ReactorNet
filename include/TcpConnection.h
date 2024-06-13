#ifndef TCPCONNECTION_HPP
#define TCPCONNECTION_HPP

#endif // TCPCONNECTION_HPP

#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

// #define MSG_SEND_AUTO
class TcpConnection
{

public:
    TcpConnection(int fd, struct EventLoop *evLoop);

    // 资源释放, 这个函数将在distpatcher的源文件的remove函数中通过回调的方式被调用

    ~TcpConnection();

    static int processRead(void* arg);
    static int processWrite(void* arg);
    static int destroy(void* arg);

private:
    EventLoop *m_evLoop;
    Channel *m_channel;
    Buffer *m_readBuf;
    Buffer *m_writeBuf;
    std::string m_name;
    HttpRequest *m_request;
    HttpResponse *m_response;
};
