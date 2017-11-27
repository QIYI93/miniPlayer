#ifndef PKTANDFRAMEQUEUE_H
#define PKTANDFRAMEQUEUE_H


#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable> 

extern "C"
{
#include <libavformat/avformat.h>
}

struct PacketQueue
{
    std::queue<AVPacket> m_queue;
    int m_size = 0;
    int m_maxElements = 0;
    bool m_readyToDequeue = false;
    std::mutex m_mutex;
    std::condition_variable m_cond;

    PacketQueue() = default;
    ~PacketQueue();

    bool enQueue(const AVPacket *packet);
    bool deQueue(AVPacket *packet, bool block = true);
};


struct FrameQueue
{
    std::queue<AVFrame> m_queue;
    int m_maxElements = 0;
    bool m_readyToDequeue = false;
    std::mutex m_mutex;
    std::condition_variable m_cond;

    FrameQueue() = default;
    ~FrameQueue();

    bool enQueue(const AVFrame *frame);
    bool deQueue(AVFrame *frame, bool block = true);
};


#endif