#ifndef MEDIAMAINCONTROL_H
#define MEDIAMAINCONTROL_H

#include <stdint.h>
#include <mutex>
#include "pktAndFramequeue.h"

class AVCodec;
class SwsContext;
class MediaDisplay;
class AVFormatContext;
class AVCodecContext;
class SwrContext;
class MediaMainControl
{
public:
    static MediaMainControl* getInstance();
    bool openFile(const char *file);
    void closeFile();

    void play();
    void pause();
    void stop();

    int64_t getDurationTime() { return m_totalTimeMS; }
    int getVideoStreamIndex() { return m_videoStreamIndex; }
    int getAudioStreamIndex() { return m_audioStreamIndex; }
    AVFrame* convertFrametoYUV420(AVFrame* src, const int width, const int height); //Do not manage returned buffer.
    bool convertFrametoPCM(AVFrame* src, uint8_t *des, int len);

private:
    MediaMainControl();
    ~MediaMainControl();

    void initSeparatePktThread();
    void initDecodePktThread();
    void cleanPktQueue();
    void cleanFrameQueue();

private:
    AVFormatContext *m_formatCtx = nullptr;
    AVCodec *m_videoCodec = nullptr;
    AVCodec *m_audioCodec = nullptr;
    AVCodecContext *m_videoCodecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;

    SwsContext *m_swsCtx = nullptr;
    SwrContext *m_swrCtx = nullptr;

    PacketQueue *m_videoPktQueue = nullptr;
    PacketQueue *m_audioPktQueue = nullptr;
    FrameQueue *m_videoFrameQueue = nullptr;
    FrameQueue *m_audioFrameQueue = nullptr;

    char *m_errMsgBuffer = nullptr;

    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    int m_frameWidth = 0;
    int m_frameHeight = 0;
    int m_fps = 0;
    int64_t m_totalTimeMS = -1;

    std::mutex m_mutex;

    bool m_noPktToSperate = false;
};

#endif
