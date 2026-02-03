#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H


#include<queue>
#include<mutex>
#include<condition_variable>

//包含ffmpeg头文件，要存储AVPacket
extern"C"{
    #include "libavcodec/avcodec.h"
}

class PacketQueue
{
    //线程安全队列

public:
    PacketQueue();
    ~PacketQueue();

    //入队（生产者调用）
    void push(AVPacket* packet);

    //出队（消费者调用）
    AVPacket* pop();

    //获得队列大小
    int size();

    //清空队列
    void clear();

private:
    std::queue<AVPacket*> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;

};

#endif // PACKETQUEUE_H
