#ifndef MEDIAMAINCONTROL_H
#define MEDIAMAINCONTROL_H

#include <stdint.h>
#include "separatePkt.h"

//struct AudioState
//{
//    const uint32_t BUFFER_SIZE;// 缓冲区的大小
//    PacketQueue audioq;
//    uint8_t *audioBuff;       // 解码后数据的缓冲空间
//    uint32_t audioBuffSize;  // buffer中的字节数
//    uint32_t audioBuffIindex; // buffer中未发送数据的index
//    int audioStream;          // audio流index
//    AVCodecContext *audio_ctx; // 已经调用avcodec_open2打开
//
//    AudioState(AVCodecContext *audio_ctx, int audio_stream);
//    ~AudioState();
//};
//
//struct VideoState
//{
//    PacketQueue* videoq;        // 保存的video packet的队列缓存
//
//    int video_stream;          // index of video stream
//    AVCodecContext *video_ctx; // have already be opened by avcodec_open2
//
//    //FrameQueue frameq;         // 保存解码后的原始帧数据
//    AVFrame *frame;
//    AVFrame *displayFrame;
//
//    VideoState();
//    ~VideoState();
//};
//
//struct MediaState
//{
//    AudioState *audio;
//    VideoState *video;
//    AVFormatContext *pFormatCtx;
//
//    char* filename;
//    //bool quit;
//    MediaState(char *filename);
//    ~MediaState();
//
//    bool openInput();
//};

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
    void cleanQueue();
    void msgOutput(const char*);

private:
    AVFormatContext *m_formatCtx = nullptr;
    AVCodec *m_videoCodec = nullptr;
    AVCodec *m_audioCodec = nullptr;
    AVCodecContext *m_videoCodecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;

    PacketQueue *m_videoPktQueue = nullptr;
    PacketQueue *m_audioPktQueue = nullptr;

    char *m_errMsgBuffer = nullptr;

    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    int64_t m_totalTimeMS = -1;
};




#endif