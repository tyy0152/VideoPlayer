#include "packetqueue.h"

PacketQueue::PacketQueue()
{

}

PacketQueue::~PacketQueue()
{

}

void PacketQueue::push(AVPacket *packet)
{
    //上锁
    std::unique_lock<std::mutex> lock(m_mutex);

    m_queue.push(packet);
    //通知任意一个消费者
    m_cond.notify_one();
}

AVPacket *PacketQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while(m_queue.empty()){
        m_cond.wait(lock);
    }

    AVPacket* pkt=m_queue.front();
    m_queue.pop();
    return pkt;

}

int PacketQueue::size()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void PacketQueue::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while(!m_queue.empty()){
        AVPacket* pkt=m_queue.front();
        m_queue.pop();
        av_packet_free(&pkt);
    }
}


