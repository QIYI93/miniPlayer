#include "pktAndFramequeue.h"


bool PacketQueue::enQueue(const AVPacket *packet)
{
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0)
        return false;
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_maxElements <= m_queue.size())
    {
        m_cond.wait(lock);
    }
    m_queue.push(*pkt);
    m_size += pkt->size;
    m_readyToDequeue = true;
    m_cond.notify_all();
    return true;

}

bool PacketQueue::deQueue(AVPacket *packet, bool block)
{
    bool ret = false;
    std::unique_lock<std::mutex> lock(m_mutex);
    for (;;)
    {
        if (!m_queue.empty())
        {
            if (av_packet_ref(packet, &m_queue.front()) < 0)
            {
                ret = false;
                break;
            }
            AVPacket pkt = m_queue.front();
            m_queue.pop();
            m_cond.notify_all();
            av_packet_unref(&pkt);
            m_size -= packet->size;

            ret = true;
            break;
        }
        else if (!block)
        {
            ret = false;
            break;
        }
        else
        {
            while (!m_readyToDequeue)
            {
                m_cond.wait(lock);
            }
        }
    }
    return ret;
}

PacketQueue::~PacketQueue()
{
    while (!m_queue.empty())
    {
        AVPacket pkt = m_queue.front();
        m_queue.pop();
        av_packet_unref(&pkt);
    }
}

bool FrameQueue::enQueue(const AVFrame *frame_)
{
    AVFrame *frame = av_frame_alloc();
    if(av_frame_ref(frame, frame_) < 0)
        return false;
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_maxElements <= m_queue.size())
    {
        m_cond.wait(lock);
    }
    m_queue.push(*frame);
    m_readyToDequeue = true;
    m_cond.notify_all();
    return true;

}

bool FrameQueue::deQueue(AVFrame *frame, bool block)
{
    bool ret = false;
    std::unique_lock<std::mutex> lock(m_mutex);
    for (;;)
    {
        if (!m_queue.empty())
        {
            if (av_frame_ref(frame, &m_queue.front()) < 0)
            {
                ret = false;
                break;
            }
            AVFrame frame_ = m_queue.front();
            m_queue.pop();
            m_cond.notify_all();
            av_frame_unref(&frame_);

            ret = true;
            break;
        }
        else if (!block)
        {
            ret = false;
            break;
        }
        else
        {
            while (!m_readyToDequeue)
            {
                m_cond.wait(lock);
            }
        }
    }
    return ret;
}

FrameQueue::~FrameQueue()
{
    while (!m_queue.empty())
    {
        AVFrame frame = m_queue.front();
        m_queue.pop();
        av_frame_unref(&frame);
    }
}