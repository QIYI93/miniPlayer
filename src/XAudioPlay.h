#ifndef XAUDIOPLAY_H
#define XAUDIOPLAY_H

#include <xaudio2.h>


#define DEF_MaxBufferCount 4
#define DEF_StreamingBufferSize 40960

class XAudioPlay;
class VoiceCallBack :public IXAudio2VoiceCallback
{
public:
    VoiceCallBack();
    VoiceCallBack(VoiceCallBack &) = delete;
    ~VoiceCallBack();

    void STDMETHODCALLTYPE OnBufferEnd(void *pBufferContext);

    void STDMETHODCALLTYPE OnBufferStart(void *pBufferContext) {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
    void STDMETHODCALLTYPE OnStreamEnd() {}
    void STDMETHODCALLTYPE OnLoopEnd(void *pBufferContext) {}
    void STDMETHODCALLTYPE OnVoiceError(void *pBufferContext, HRESULT Error) {}

public:
    int m_count;
    int m_lastContext;
    int m_currentContext;
    HANDLE m_hBufferEndEvent;
    XAudioPlay *m_audioPlay;
    XAUDIO2_VOICE_STATE m_state;


};

class XAudioPlay
{

public:
    friend class VoiceCallBack;
    XAudioPlay();
    XAudioPlay(XAudioPlay&) = delete;
    ~XAudioPlay();
    bool isValid() { return m_isValid; }
    bool setFormat(int bytes, int channels, int freq);
    void setBufferSize(int maxBufferCount, int streamingBufferSize);
    void pause();
    void startPlaying();
    void stopPlaying();
    void readData(unsigned char *buffer, int length); //blocking
    int  getDuration();

private:
    void reset();
    void deleteBuffer();

    static XAUDIO2_BUFFER makeXAudio2Buffer(const BYTE *pBuffer, int BufferSize);

private:
    IXAudio2* m_XAudio2 = nullptr;
    IXAudio2MasteringVoice* m_masterVoice = nullptr;
    VoiceCallBack m_voiceCallBack;
    XAUDIO2_BUFFER *m_audio2Buffer = nullptr;
    WAVEFORMATEX m_waveFormat;
    IXAudio2SourceVoice *m_sourceVoice = nullptr;

    int m_maxBufferCount;
    int m_streamingBufferSize;
    unsigned char ** m_audioData = nullptr;

    bool m_isPlaying;
    bool m_isValid = false;

    int m_readingBufferNumber = 0;
    int m_writingBufferNumber = 0;
    int m_unReadingBufferCount;


    int m_unProcessedBufferCount;
    int m_writingPosition;
};


#endif
