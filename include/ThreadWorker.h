#ifndef THREADWORKER_HPP
#define THREADWORKER_HPP

#endif // THREADWORKER_HPP

#include <pthread.h>
#include "EventLoop.h"
#include <string>
#include <condition_variable>

// 定义子线程对应的结构体
class WorkerThread
{

public:
    //index是指当前线程在线程池里的编号
    WorkerThread(int index);
    ~WorkerThread();


    // 启动线程
    void run();
    inline EventLoop* getEvLoop(){
        return m_evLoop;
    }
private:
    void* running();

private:
    std::thread* m_thread;
    std::thread::id m_threadID;
    std::string m_name;
    std::mutex m_mutex;             // 互斥锁
    std::condition_variable m_cond; // 条件变量
    EventLoop *m_evLoop;            // 反应堆模型
};
