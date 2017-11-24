#ifndef MEDIAMAINCONTROL_H
#define MEDIAMAINCONTROL_H

#include <stdint.h>
#include "separatePkt.h"

//struct AudioState
//{
//    const uint32_t BUFFER_SIZE;// �������Ĵ�С
//    PacketQueue audioq;
//    uint8_t *audioBuff;       // ��������ݵĻ���ռ�
//    uint32_t audioBuffSize;  // buffer�е��ֽ���
//    uint32_t audioBuffIindex; // buffer��δ�������ݵ�index
//    int audioStream;          // audio��index
//    AVCodecContext *audio_ctx; // �Ѿ�����avcodec_open2��
//
//    AudioState(AVCodecContext *audio_ctx, int audio_stream);
//    ~AudioState();
//};
//
//struct VideoState
//{
//    PacketQueue* videoq;        // �����video packet�Ķ��л���
//
//    int video_stream;          // index of video stream
//    AVCodecContext *video_ctx; // have already be opened by avcodec_open2
//
//    //FrameQueue frameq;         // ���������ԭʼ֡����
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