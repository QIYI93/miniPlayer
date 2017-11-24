#ifndef MEDIAMAINCONTROL_H
#define MEDIAMAINCONTROL_H

#include <stdint.h>
#include "pktAndFramequeue.h"

class AVFormatContext;
class AVCodec;
class AVCodecContext;
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

private:
    MediaMainControl();
    ~MediaMainControl();

    void playAudio();
    void playVideo();
    void initSeparatePktThread();
    void initDecodePktThread();
    void cleanPktQueue();
    void cleanFrameQueue();
    void msgOutput(const char*);

private:
    AVFormatContext *m_formatCtx = nullptr;
    AVCodec *m_videoCodec = nullptr;
    AVCodec *m_audioCodec = nullptr;
    AVCodecContext *m_videoCodecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;

    PacketQueue *m_videoPktQueue = nullptr;
    PacketQueue *m_audioPktQueue = nullptr;
    FrameQueue *m_videoFrameQueue = nullptr;
    FrameQueue *m_audioFrameQueue = nullptr;

    char *m_errMsgBuffer = nullptr;

    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    int64_t m_totalTimeMS = -1;
};




#endif