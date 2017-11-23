#ifndef MEDIAMAINCONTROL_H
#define MEDIAMAINCONTROL_H

#include <stdint.h>

class AVFormatContext;
class AVCodec;
class AVCodecContext;
class MediaMainControl
{
public:
    static MediaMainControl* getInstance();
    bool openFile(const char *file);
    void closeFile();
    void initSeparatePkt();

    int64_t getDurationTime() { return m_totalTimeMS; }
    int getVideoStreamIndex() { return m_videoStreamIndex; }
    int getAudioStreamIndex() { return m_audioStreamIndex; }

protected:


private:
    MediaMainControl();
    ~MediaMainControl();

private:
    AVFormatContext *m_formatCtx = nullptr;
    AVCodec *m_videoCodec = nullptr;
    AVCodec *m_audioCodec = nullptr;
    AVCodecContext *m_videoCodecCtx = nullptr;
    AVCodecContext *m_audioCodecCtx = nullptr;

    char *m_errMsgBuffer = nullptr;
    int64_t m_totalTimeMS = -1;

    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;

};




#endif