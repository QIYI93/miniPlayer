#ifndef MEDIAMAINCONTROL_H
#define MEDIAMAINCONTROL_H

#include <stdint.h>
#include <mutex>
#include <atomic>
#include <memory>
#include "pktAndFramequeue.h"

enum class RenderType :int
{
    USING_D3D9,  // If media file's codec format inputed is DXVA2.0 supported, then use it.
    USING_SDL,      // Using software decoder to display and using SDL library to display.
};


typedef struct AudioParams {
    int freq;
    int channels;
    int32_t channelLayout;
    enum AVSampleFormat fmt;
    int frameSize;
    int bytesPerSec;
} AudioParams;

enum class GraphicDataType :int
{
    YUV420 = 0,
    BGRA,
};

typedef struct DecodeContext {
    AVBufferRef *hwDeviceRef;
} DecodeContext;

class Dxva2Wrapper;
class AVCodec;
class SwsContext;
class MediaDisplay;
class AVFormatContext;
class AVCodecContext;
class SwrContext;
class IDirect3DSurface9;
class MediaMainControl
{
public:
    //static MediaMainControl* getInstance();
    MediaMainControl(RenderType type = RenderType::USING_SDL);
    ~MediaMainControl();

    bool openFile(const char *file);
    void closeFile();

    void play();
    void stop();

    int32_t getDurationTime() { return m_totalTimeMS; }

    bool getGraphicData(GraphicDataType type, int width, int height, void *data, const uint32_t size, int *lineSize, int32_t *pts);
    bool getSurface(int32_t *pts, void** surface); //only use in DXVA mode
    bool getPCMData(void *data, const uint32_t size, const AudioParams para, int32_t *pts, int32_t *outLen);
    bool isDXVASupport() { return m_isDXVASupport; }
    bool isAudioFrameEmpty() { return m_audioFrameQueue->m_noMorePktToDecode && m_audioFrameQueue->m_queue.empty(); }
    bool isVideoFrameEmpty() { return m_videoFrameQueue->m_noMorePktToDecode && m_videoFrameQueue->m_queue.empty(); }

private:

    void initPktQueue();
    void initFrameQueue();
    static void initSeparatePktThread(void *mainCtrl);
    static void initDecodePktThread(void *mainCtrl);
    void cleanPktQueue();
    void cleanFrameQueue();

    int getVideoStreamIndex() { return m_videoStreamIndex; }
    int getAudioStreamIndex() { return m_audioStreamIndex; }
    bool setVideoDecoder();
    bool setAudioDecoder();
    int32_t getVideoFramPts(AVFrame *pframe);
    int32_t getAudioFramPts(AVFrame *pframe);

private:
    AVFormatContext *m_formatCtx = nullptr;
    AVCodec *m_videoCodec = nullptr;
    AVCodec *m_audioCodec = nullptr;
    AVCodecContext *m_videoCodecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    SwrContext *m_swrCtx = nullptr;

    DecodeContext m_decode = { NULL };
    std::unique_ptr<Dxva2Wrapper> m_dxva2Wrapper = nullptr;

    AVFrame* m_audioFrameRaw = nullptr;
    AVFrame* m_videoFrameRaw = nullptr;

    PacketQueue *m_videoPktQueue = nullptr;
    PacketQueue *m_audioPktQueue = nullptr;
    FrameQueue *m_videoFrameQueue = nullptr;
    FrameQueue *m_audioFrameQueue = nullptr;

    MediaDisplay *m_mediaDisplay = nullptr;

    char *m_errMsgBuffer = nullptr;
    std::string m_file;

    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    int m_frameWidth = 0;
    int m_frameHeight = 0;
    int m_fps = 0;
    int32_t m_totalTimeMS = -1;
    int32_t m_videoClock = 0;
    int32_t m_audioClock = 0;
    std::mutex m_mutex;

    std::atomic_bool m_noPktToSperate = false;
    std::atomic_bool m_isQuit = false;
    std::thread m_separatePktThread;
    std::thread m_decodeVideoThread;
    std::thread m_decodeAudioThread;

    bool m_isDXVASupport = false;
    RenderType m_renderType;
};

#endif
