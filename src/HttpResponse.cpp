#include "../include/HttpResponse.h"

HttpResponse::HttpResponse()
{
    m_statusCode = StatusCode::Unknown;
    m_headers.clear();
    m_fileName = "";
    sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const std::string &key, const std::string &value)
{

    if (key.empty() || value.empty())
    {
        return;
    }
    m_headers.insert(std::make_pair(key, value));
}

void HttpResponse::prepareMsg(Buffer *sendBuf, int socket)
{
    // 状态行
    char tmp[1024] = {0};
    int code = static_cast<int>(m_statusCode);
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", code,
            m_info.at(code).data());
    sendBuf->append(tmp);

    // 响应头
    for (auto it: m_headers)
    {
        sprintf(tmp, "%s: %s\r\n", it.first.data(), it.second.data());
        sendBuf->append(tmp);
    }
    // 空行
    sendBuf->append("\r\n");
#ifndef MSG_SEND_AUTO
    sendBuf->sendData(socket);
#endif

    // 回复的数据
    // 这个函数指针指的其实就是sendFil和sendDir
    sendDataFunc(m_fileName.data(), sendBuf, socket);
}
