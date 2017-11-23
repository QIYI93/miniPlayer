#ifndef SEPARATEPKE_H
#define SEPARATEPKE_H


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

    bool enQueue(const AVPacket *packet);
    bool deQueue(AVPacket *packet, bool block = true);
};


struct SeparatePkt
{
    bool m_quit = false;

};



#endif