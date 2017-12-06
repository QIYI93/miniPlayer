#include "pktAndFramequeue.h"


bool PacketQueue::enQueue(const AVPacket *packet)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0)
        return false;
    while (m_maxElements <= m_queue.size())
    {
        m_condEnQueue.wait(lock);
    }
    m_queue.push(pkt);
    m_size += pkt->size;
    m_condDeQueue.notify_all();
    return true;

}

bool PacketQueue::deQueue(AVPacket *packet, bool block)
{
    bool ret = true;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty())
    {
        m_condDeQueue.wait(lock);
    }
    if (av_packet_ref(packet, m_queue.front()) < 0)
    {
        ret = false;
    }
    AVPacket *pkt = m_queue.front();
    av_packet_unref(pkt);
    m_queue.pop();
    m_condEnQueue.notify_all();

    return ret;
}

PacketQueue::~PacketQueue()
{
    while (!m_queue.empty())
    {
        AVPacket *pkt = m_queue.front();
        m_queue.pop();
        av_packet_unref(pkt);
    }
}

bool FrameQueue::enQueue(const AVFrame *frame_)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    AVFrame *frame = av_frame_alloc();
    if(av_frame_ref(frame, frame_) < 0)
        return false;
    while (m_maxElements <= m_queue.size())
    {
        m_condEnQueue.wait(lock);
    }
    m_queue.push(frame);
    m_condDeQueue.notify_all();
    return true;
}

bool FrameQueue::deQueue(AVFrame *frame, bool block)
{
    bool ret = true;
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_queue.empty())
    {
        m_condDeQueue.wait(lock);
    }
    if (av_frame_ref(frame, m_queue.front()) < 0)
    {
        ret = false;
    }
    AVFrame *frame_ = m_queue.front();
    av_frame_unref(frame_);
    m_queue.pop();
    m_condEnQueue.notify_all();

    return ret;
}

FrameQueue::~FrameQueue()
{
    while (!m_queue.empty())
    {
        AVFrame *frame = m_queue.front();
        m_queue.pop();
        av_frame_unref(frame);
    }
}