#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#endif // HTTPREQUEST_HPP

#include "Buffer.h"
#include "HttpResponse.h"
#include <string>
#include <map>
#include <functional>

// 请求头键值对
struct RequestHeader
{
    char *key;
    char *value;
};

// 当前解析状态
enum class HttpProcessState : char
{
    ParseReqLine,
    ParseReqHeader,
    ParseReqBody,
    ParseReqDone
};

// http结构体
class HttpRequest
{
public:
    HttpRequest();
    ~HttpRequest();

    // 重置
    void reset();
    // 获取处理状态

    inline HttpProcessState getState()
    {
        return m_curState;
    }

    inline void setState(HttpProcessState state)
    {
        m_curState = state;
    }

    // 根据key得到请求头的value
    void addHeader(const std::string key, const std::string value);

    std::string getHeader(const std::string key);

    // ***解析请求行
    bool parseHttpRequestLine(Buffer *readBuf);
    // ***解析请求头
    bool parseHttpRequestHeader(Buffer *readBuf);

    // 解析http请求协议
    bool parseHttpRequest(Buffer *readBuf, HttpResponse *response,
                          Buffer *sendBuf, int socket);

    // 处理http请求协议
    bool processHttpRequest(HttpResponse *response);

    std::string decodeMsg(std::string msg);

    const std::string getFileType(const std::string name);
    static void sendDir(const std::string& dirName, Buffer *sendBuf, int cfd);
    static void sendFile(const std::string fileName, Buffer *sendBuf, int cfd);

    inline void setMethod(std::string method)
    {
        m_method = method;
    }

    inline void setUrl(std::string url)
    {
        m_url = url;
    }

#include <functional>

    inline void setVersion(std::string version)
    {
        m_version = version;
    }

private:
    char *splitRequestLine(const char *start, const char *end,
                           const char *sub, std::function<void(std::string)> callback);

    int hexToDec(char c);
    

private:
    std::string m_method;
    std::string m_url;
    std::string m_version;
    std::map<std::string, std::string> m_reqHeaders;

    // 添加处理四个部分的标记，请求行，请求头，空行，请求数据块。
    enum HttpProcessState m_curState;
};
