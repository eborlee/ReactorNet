#include "../include/HttpRequest.h"
#include "../include/Buffer.h"
#include "../include/HttpResponse.h"
#include <string>
#include <map>
#include <functional>
#include <cstring>
#include <cassert>
#include <sys/stat.h>
#include <dirent.h> // Include the necessary header file
#include <fcntl.h>
#include <unistd.h>
#include "../include/Log.h"

HttpRequest::HttpRequest()
{
    reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
    m_curState = HttpProcessState::ParseReqLine;
    m_method = m_url = m_version = std::string();
    m_reqHeaders.clear();
}

void HttpRequest::addHeader(const std::string key, const std::string value)
{
    if (key.empty() || value.empty())
    {
        return;
    }
    m_reqHeaders[key] = value;
}

std::string HttpRequest::getHeader(const std::string key)
{
    auto item = m_reqHeaders.find(key);
    if (item != m_reqHeaders.end())
    {
        return item->second;
    }
    return std::string();
}

bool HttpRequest::parseHttpRequestLine(Buffer *readBuf)
{
    // 读出请求行
    // 保存字符串起始和结束地址
    // http协议里换行是\r\n
    char *end = readBuf->findCRLF();
    char *start = readBuf->getData();
    int lineSize = end - start;
    if (lineSize)
    {

        auto methodFunc = std::bind(&HttpRequest::setMethod, this,
                                    std::placeholders::_1);
        start = splitRequestLine(start, end, " ", methodFunc);

        auto urlFunc = std::bind(&HttpRequest::setUrl, this,
                                 std::placeholders::_1);
        start = splitRequestLine(start, end, " ", urlFunc);

        auto versionFunc = std::bind(&HttpRequest::setVersion, this,
                                     std::placeholders::_1);
        splitRequestLine(start, end, nullptr, versionFunc);
        // get /xxx/xx.txt http/1.1
        // 请求方式

        // 为解析请求头准备
        readBuf->readPosIncrease(lineSize);
        readBuf->readPosIncrease(2);

        // 修改状态
        setState(HttpProcessState::ParseReqHeader);
        return true;
    }
}

bool HttpRequest::parseHttpRequestHeader(Buffer *readBuf)
{
    // 只处理一行，然后多次调用
    char *end = readBuf->findCRLF();
    if (end != NULL)
    {
        char *start = readBuf->getData();
        int lineSize = end - start;
        // 基于搜索字符串
        char *middle = static_cast<char *>(memmem(start, lineSize, ": ", 2));
        // 此时middle指向冒号
        if (middle != NULL)
        {
            int keyLen = middle - start;
            int valueLen = end - middle - 2;
            if (keyLen > 0 && valueLen > 0)
            {
                std::string key(start, keyLen);
                std::string value(middle + 2, valueLen);

                addHeader(key, value);
            }

            // 移动读数据的位置
            readBuf->readPosIncrease(lineSize);
            readBuf->readPosIncrease(2);
        }
        else
        {
            // 此时搜索到了第三部分的\r\n,  跳过
            readBuf->readPosIncrease(2);
            // 修改解析状态, 取决于get还是post。如果是get，已经结束了. 暂时忽略post
            setState(HttpProcessState::ParseReqDone);
            Debug("解析请求头结束, 已设置ParseReqDone标记");
        }
        return true;
    }
    return false;
}

bool HttpRequest::parseHttpRequest(Buffer *readBuf,
                                   HttpResponse *response, Buffer *sendBuf, int socket)
{
    Debug("开始解析Http请求...");
    bool flag = true;
    while (m_curState != HttpProcessState::ParseReqDone)
    {
        switch (m_curState)
        {
        case HttpProcessState::ParseReqLine:
            flag = parseHttpRequestLine(readBuf);
            break;
        case HttpProcessState::ParseReqHeader:
            flag = parseHttpRequestHeader(readBuf);
            break;
        case HttpProcessState::ParseReqBody:
            // 如果是post请求。情况更加复杂。本项目不涉及
            break;
        default:
            break;
        }
        if (!flag)
        {
            return flag;
        }
        // 判断是否解析完毕，如是，准备回复数据
        if (m_curState == HttpProcessState::ParseReqDone)
        {
            // 1. 根据解析的原始数据，对请求进行处理
            processHttpRequest(response);
            // 2. 组织响应数据
            response->prepareMsg(sendBuf, socket);
        }
    }
    Debug("解析Http请求结束...");

    m_curState = HttpProcessState::ParseReqLine; // 还原状态，以便处理第二条及以后的请求。

    return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse *response)
{
    if (strcasecmp(m_method.data(), "get") != 0)
    {
        // 值相等时返回0
        // 不处理get之外的请求
        return -1;
    }
    m_url = decodeMsg(m_url); // 覆盖, 因为解码之后一定比utf8格式的短
    Debug("新的url: %s", m_url.data());

    // 处理客户端请求的静态资源 dir or files
    const char *file = NULL;
    // 首先判断是否传入的路径是否是资源根路径
    // 此处他说要把服务器进程切换到服务器给客户端准备的资源根目录。而这个根目录是最最外层的main函数启动的时候指定的。

    if (strcmp(m_url.data(), "/") == 0)
    {
        file = "./";
    }
    else
    {
        file = m_url.data() + 1; // char数组指针+1跳过/
    }
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在 回复 404
        response->setFileName("404.html");
        response->setStatusCode(StatusCode::NotFound);
        // 响应头
        response->addHeader("Content-type", getFileType(".html"));

        response->sendDataFunc = sendFile;                                     //  这个函数见之前
        return 0;
    }

    response->setFileName(file);
    response->setStatusCode(StatusCode::OK);

    // 判断文件类型
    if (S_ISDIR(st.st_mode))
    {
        //  把这个目录中的内容发送给客户端
        // 响应头
        response->addHeader("Content-type", getFileType(".html")); //  getFileType这个函数见之前
        response->sendDataFunc = sendDir;                                      //  这个函数见之前
        return 0;
    }
    else
    {
        // 把文件的内容发送给客户端
        // 响应头
        char tmp[128];
        sprintf(tmp, "%ld", st.st_size);
        response->addHeader("Content-type", getFileType(file)); //  getFileType这个函数见之前
        response->addHeader("Content-length", std::to_string(st.st_size));
        response->sendDataFunc = sendFile; //  这个函数见之前
        return 0;
    }

    return false;
}

std::string HttpRequest::decodeMsg(std::string msg)
{
    const char *from = msg.data();
    std::string str = std::string();
    for (; *from != '\0'; ++from)
    {
        // isxdigit是判断是否是16进制的数字
        // 比如说 Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 3个字符合成一个字符，即变成了原始数据
            str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));
            // 跳过 from[1] 和 from[2]
            from += 2;
        }
        else
        {
            // Linux这5个字符就不满足
            str.append(1, *from);
        }
    }
    // 为什么要添加\0？
    // str.append(1, '\0');
    return str;
}

int HttpRequest::hexToDec(char c)
{
    // 16进制转10
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

char *HttpRequest::splitRequestLine(const char *start, const char *end,
                                    const char *sub, std::function<void(std::string)> callback)
{
    // 本函数传进来的callback就是三个set方法

    char *space = const_cast<char *>(end);
    if (sub != NULL)
    {
        space = (char *)memmem(start, end - start, sub, strlen(sub));
        assert(space != NULL);
    }
    int length = space - start;
    callback(std::string(start, length));
    return space + 1;
}

const std::string HttpRequest::getFileType(const std::string name)
{
    // 自右向左找
    const char *dot = strrchr(name.data(), '.'); // 查找文件名中最后一个点的位置
    if (dot == NULL)
    {
        return "application/octet-stream"; // 默认的二进制流类型，用于未知文件类型
    }

    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
    {
        return "text/html; charset=utf-8";
    }
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
    {
        return "image/jpeg";
    }
    if (strcmp(dot, ".gif") == 0)
    {
        return "image/gif";
    }
    if (strcmp(dot, ".png") == 0)
    {
        return "image/png";
    }
    if (strcmp(dot, ".css") == 0)
    {
        return "text/css; charset=utf-8";
    }
    if (strcmp(dot, ".au") == 0)
    {
        return "audio/basic";
    }
    if (strcmp(dot, ".wav") == 0)
    {
        return "audio/wav";
    }
    if (strcmp(dot, ".mp3") == 0)
    {
        return "audio/mpeg";
    }
    if (strcmp(dot, ".mp4") == 0)
    {
        return "video/mp4";
    }
    if (strcmp(dot, ".avi") == 0)
    {
        return "video/x-msvideo";
    }
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
    {
        return "video/quicktime";
    }
    if (strcmp(dot, ".js") == 0)
    {
        return "application/javascript; charset=utf-8";
    }
    if (strcmp(dot, ".json") == 0)
    {
        return "application/json; charset=utf-8";
    }
    if (strcmp(dot, ".xml") == 0)
    {
        return "application/xml; charset=utf-8";
    }
    if (strcmp(dot, ".zip") == 0)
    {
        return "application/zip";
    }
    if (strcmp(dot, ".pdf") == 0)
    {
        return "application/pdf";
    }
    // 更多的文件类型可以根据需要添加到这里

    return "application/octet-stream"; // 默认的二进制流类型，用于未知文件类型
}

void HttpRequest::sendDir(const std::string& dirName, Buffer *sendBuf, int cfd)
{
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName.data());

    struct dirent **namelist; // 该指针指向的内存地址是scandir这个函数完成分配的

    int num = scandir(dirName.data(), &namelist, NULL, alphasort);
    for (int i = 0; i < num; ++i)
    {
        // 取出文件名 namelist指向一个指针数组-> struct dirent* tmp[];
        char *name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dirName.data(), name);
        stat(subPath, &st);
        // 判断是目录还是文件
        if (S_ISDIR(st.st_mode))
        {
            // a标签 <a href="">name</a>
            // 跳转时href中最后带/表示访问目录，不带/表示打开文件
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }
        else
        {
            sprintf(buf + strlen(buf),
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }
        // send(cfd, buf, strlen(buf), 0);
        sendBuf->append(buf);
#ifndef MSG_SEND_AUTO
        sendBuf->sendData(cfd);
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    // send(cfd, buf, strlen(buf), 0);
    sendBuf->append(buf);
#ifndef MSG_SEND_AUTO
    sendBuf->sendData(cfd);
#endif
    free(namelist);
    return;
}

void HttpRequest::sendFile(const std::string fileName, Buffer *sendBuf, int cfd)
{
    // 打开文件
     // Add this line to include the necessary header file

    int fd = open(fileName.data(), O_RDONLY);
    // 通过断言方式判断文件是否打开成功
    assert(fd > 0);
    // 方式1
    while (1)
    {
        // 每次读1k
        char buf[1024];
         // Add this line to include the necessary header file

        int len = read(fd, buf, sizeof buf);
        if (len > 0)
        {
            // send(cfd, buf, len,0 );
            //  此处不应该用bufferAppendString因为这个函数默认传进来的buf结尾自带\0而此处没有
            sendBuf->append(buf, len);
#ifndef MSG_SEND_AUTO
            sendBuf->sendData(cfd);
#endif
            // usleep(10); // 非常重要，短暂的休息以降低接收端接收压力，接收端缓存可能会满，又或者浏览器无法一下子把传来的内容解析
            //  但是此处这里要注释掉，因为bufferSendData里做了
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            close(fd);
            perror("read");
        }
    }

    close(fd);
}

