#ifndef BUFFER_HPP
#define BUFFER_HPP

#endif // BUFFER_HPP

class Buffer
{
public:
    // 初始化buffer
    Buffer(int size);
    // 析构函数
    ~Buffer();

    // 扩容
    void extendRoom(int size);
    // 得到剩余的可写的内存容量
    inline int getWriteableSize(){
        return m_capacity - m_writePos;
    }
    // 得到剩余的可读的内存容量
    inline int getReadableSize(){
        return m_writePos - m_readPos; // 未读区域末-首
    }
    // 写内存 1. 直接写 2.接收套接字数据
    int append(const char *data, int size);
    int append(const char *data);

    int socketRead(int fd);

    // 根据\r\n取出一行，即找出其在数据块中的位置，返回该位置
    char *findCRLF();

    //  发送数据
    int sendData(int socket);

    inline char *getData(){
        return m_data + m_readPos;
    }

    inline int readPosIncrease(int count){
        m_readPos += count;
        return m_readPos;
    }

    

private:
    // 指向内存的指针
    char *m_data;
    int m_capacity;
    // 偏移量
    int m_readPos = 0 ;
    int m_writePos = 0;
};
