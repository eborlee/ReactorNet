#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP


#include "Buffer.h"
#include <functional>
#define ResHeadSize 16
#include <map>

enum class StatusCode
{
    Unknown,
    OK = 200,
    MovedPermanently = 301, // 永久重定向
    MovedTemporarily = 302, // 临时重定向
    BadRequest = 400,       // 错误请求
    NotFound = 404          // 请求资源不存在
};

// 定义一个回调函数指针，用来组织回复的数据块
typedef void (*responseBody)(const char *fileName, struct Buffer *sendBuf, int socket);

// 定义结构体
class HttpResponse
{
public:
    // 初始化
    HttpResponse();
    ~HttpResponse();

    // 添加响应头
    void addHeader(const std::string &key, const std::string &value);
    // 组织http响应数据
    void prepareMsg(Buffer *sendBuf, int socket);

    inline void setStatusCode(StatusCode code)
    {
        m_statusCode = code;
    }

    inline void setFileName(const std::string &name)
    {
        m_fileName = name;
    }

public:
    std::function<void(const char *, Buffer *, int)> sendDataFunc;

private:
    // 状态行: 状态码，状态描述
    StatusCode m_statusCode;

    // 响应头 - 键值对
    std::map<std::string, std::string>  m_headers;


    const std::map<int, std::string> m_info = {
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"}};

    std::string m_fileName;
};


#endif // HTTPRESPONSE_HPP