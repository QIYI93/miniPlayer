#include "separatePkt.h"


bool PacketQueue::enQueue(const AVPacket *packet)
{
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0)
        return false;
    std::lock_guard<std::mutex> lock(m_mutex);
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
