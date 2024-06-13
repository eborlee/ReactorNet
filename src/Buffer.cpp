#include "../include/Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <strings.h>

Buffer::Buffer(int size) : m_capacity(size)
{
    m_data = (char *)malloc(size);
    bzero(m_data, size);
}

Buffer::~Buffer()
{
    if (m_data)
        free(m_data);
}

void Buffer::extendRoom(int size)
{
    // 1.内存够用，不需要扩容
    if (getWriteableSize() >= size)
    {
        return;
    }

    // 2。 内存需要合并够用，不需要扩容
    // 剩余的可写的内存+ 已读的内存 》size
    else if (m_readPos + getWriteableSize() >= size)
    {
        int readable = getReadableSize();
        // 第一个参数：数据移动到哪里去，即data首地址；第二个参数，要移动的数据的地址；第三个参数，要移动的数据的长度
        memcpy(m_data, m_data + m_readPos, readable);
        // 更新位置
        m_readPos = 0;
        m_writePos = readable;
    }
    // 3. 内存不够用，扩容
    else
    {
        void *temp = realloc(m_data, m_capacity + size);
        if (temp == NULL)
        {
            return; // 失败
        }
        memset((char*)temp + m_capacity, 0, size);
        // 更新数据
        m_data = static_cast<char *>(temp);
        m_capacity += size;
    }
}

int Buffer::append(const char *data, int size)
{
    // 一般来说字符串都是以\0结束，因此可以通过strlen来判断其长度
    // 没有\0的特殊情况：二进制字符中间存在\0的情况
    // 因此还是要求函数传入一个size参数
    if (data == nullptr || size <= 0)
    {
        return -1;
    }
    // 扩容
    extendRoom(size);
    // 数据拷贝
    memcpy(m_data + m_writePos, data, size);
    m_writePos += size;
    return 0;
}

int Buffer::append(const char *data)
{
    // 处理添加字符串数据
    int size = strlen(data);
    int ret = append(data, size);
    return ret;
}

int Buffer::socketRead(int fd)
{
    // read/recv/readv 对于前两者接收数据的时候只能指定一个数组，而最后一个可以指定多个数组
    // 详情见man readv
    struct iovec vec[2];
    // 初始化数组元素
    int writeable = getWriteableSize();
    vec[0].iov_base = m_data + m_writePos;
    vec[0].iov_len = writeable;
    char *tmpbuf = (char *)malloc(40960);
    vec[1].iov_base = tmpbuf;
    vec[1].iov_len = 40960;
    int result = readv(fd, vec, 2);
    if (result == -1)
    {
        return -1;
    }
    else if (result <= writeable)
    {
        // 说明接收数据的时候全部数据都写入了buffer的data数组里了
        m_writePos += result;
    }
    else
    {
        // 此时说明buffer里只写了一部分，不够写了
        // 由于readv向buffer里写了一部分数据，因此要更新当前buffer的位置，以及计算还需要写多少；
        m_writePos = m_capacity;
        append(tmpbuf, result - writeable);
    }
    free(tmpbuf);
    return result; // 表示以供接收到了多少个字节
}

char *Buffer::findCRLF()
{
    char *ptr = static_cast<char *>(memmem(m_data + m_readPos, getReadableSize(), "\r\n", 2));
    return ptr;
}

/**
 * @brief Sends data through the specified socket.
 *
 * This function is responsible for sending data through the specified socket.
 *
 * @param socket The socket to send data through.
 * @return The number of bytes sent, or -1 if an error occurred.
 */
int Buffer::sendData(int socket)
{
    int readable = getReadableSize(); // 还有多少没处理的数据，即可读数据, 注意不是writeableSize
    if (readable > 0)
    {
        // 此处有可能出现tcp连接的broken pipe。不是代码引起的。
        int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL); // 最后一个参数设置为这个保证不被底层终止，阻止SIGPIPE信号。
        if (count)
        {
            m_readPos += count;
            usleep(1);
        }
        return count;
    }
}
