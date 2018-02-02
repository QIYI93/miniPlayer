#include "xaudioplay.h"
#include "util.h"

VoiceCallBack::VoiceCallBack()
    :m_hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    ,m_count(NULL)
    ,m_currentContext(NULL)
    ,m_lastContext(NULL)
    ,m_audioPlay(nullptr)
{
}

VoiceCallBack::~VoiceCallBack()
{
    CloseHandle(m_hBufferEndEvent);
}

void VoiceCallBack::OnBufferEnd(void *bufferContext)
{
    if (bufferContext == nullptr)
    {
        m_audioPlay->m_sourceVoice->GetState(&m_state);
        m_audioPlay->m_unProcessedBufferCount = m_state.BuffersQueued;
    }
    else
    {
        m_currentContext = *((int *)bufferContext) + 1;
        if (m_currentContext > m_lastContext)
            m_count += m_currentContext - m_lastContext;
        else if (m_currentContext < m_lastContext)
            m_count += m_audioPlay->m_maxBufferCount + m_currentContext - m_lastContext;
        m_lastContext = m_currentContext;

        m_audioPlay->m_unProcessedBufferCount -= m_count;
    }
    m_count = 0;
    while (m_audioPlay->m_unReadingBufferCount)
    {
        m_audioPlay->m_sourceVoice->SubmitSourceBuffer(&m_audioPlay->m_audio2Buffer[m_audioPlay->m_readingBufferNumber], nullptr);
        m_audioPlay->m_readingBufferNumber = (1 + m_audioPlay->m_readingBufferNumber) % m_audioPlay->m_maxBufferCount;
        m_audioPlay->m_unProcessedBufferCount++;
        m_audioPlay->m_unReadingBufferCount--;
    }

    SetEvent(m_hBufferEndEvent);
}

XAudioPlay::XAudioPlay()
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    m_isPlaying = false;
    m_maxBufferCount = 0;
    m_streamingBufferSize = 0;
    m_voiceCallBack.m_audioPlay = this;
    reset();

    //create engine
    HRESULT lRet;
    lRet = XAudio2Create(&m_XAudio2, NULL, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(lRet))
        return;

    lRet = m_XAudio2->CreateMasteringVoice(&m_masterVoice);
    if (FAILED(lRet))
        return;

    setBufferSize(DEF_MaxBufferCount, DEF_StreamingBufferSize);
    m_waveFormat.nChannels = 2; //default

    lRet = m_XAudio2->CreateSourceVoice(&m_sourceVoice,
        &m_waveFormat,
        NULL,
        XAUDIO2_DEFAULT_FREQ_RATIO,
        &m_voiceCallBack,
        nullptr,
        nullptr);

    if (FAILED(lRet))
        return;

    m_isValid = true;
}


XAudioPlay::~XAudioPlay()
{
    deleteBuffer();
    m_sourceVoice->DestroyVoice();
    m_masterVoice->DestroyVoice();
    SAFE_RELEASE(m_XAudio2);

    CoUninitialize();
}

void XAudioPlay::deleteBuffer()
{
    for (int i = 0; i < m_maxBufferCount; ++i)
    {
        SAFE_DELETE_ARRAY(m_audioData[i]);
        if (m_audio2Buffer[i].pContext)
            delete m_audio2Buffer[i].pContext;
    }
    SAFE_DELETE_ARRAY(m_audioData);
    SAFE_DELETE_ARRAY(m_audio2Buffer);
}

void XAudioPlay::setBufferSize(int maxBufferCount, int streamingBufferSize)
{
    if (m_maxBufferCount != maxBufferCount || m_streamingBufferSize != streamingBufferSize)
    {
        stopPlaying();
        deleteBuffer();

        m_maxBufferCount = maxBufferCount;
        m_streamingBufferSize = streamingBufferSize;
        m_voiceCallBack.m_lastContext = 0;
        reset();

        m_audioData = new unsigned char *[maxBufferCount];
        m_audio2Buffer = new XAUDIO2_BUFFER[maxBufferCount];
        for (int i = 0; i < maxBufferCount; ++i)
        {
            m_audioData[i] = new unsigned char[streamingBufferSize];
            m_audio2Buffer[i] = XAudioPlay::makeXAudio2Buffer(m_audioData[i], streamingBufferSize);
            *((int*)this->m_audio2Buffer[i].pContext) = i;
        }
    }
}


void XAudioPlay::reset()
{
    m_readingBufferNumber = 0;
    m_writingBufferNumber = 0;
    m_unReadingBufferCount = 0;
    m_unProcessedBufferCount = 0;
    m_writingPosition = 0;
    m_voiceCallBack.m_count = 0;
    m_voiceCallBack.m_lastContext = 0;
}

void XAudioPlay::stopPlaying()
{
    if (m_sourceVoice == nullptr)
        return;

    XAUDIO2_VOICE_STATE state;

    if (m_isPlaying)
    {
        m_isPlaying = false;
        m_sourceVoice->GetState(&state);
        while (state.BuffersQueued)
        {
            m_sourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
            m_sourceVoice->FlushSourceBuffers();
            Sleep(1);
            m_sourceVoice->GetState(&state);
        }
    }
    reset();
}

void XAudioPlay::pause()
{
    if (m_sourceVoice == nullptr)
        return;

    if (!m_isPlaying)
        return;

    m_sourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
    m_isPlaying = false;
}

void XAudioPlay::startPlaying()
{
    m_sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
    m_isPlaying = true;
}

XAUDIO2_BUFFER XAudioPlay::makeXAudio2Buffer(const BYTE *pBuffer, int BufferSize)
{
    XAUDIO2_BUFFER xAudio2Buffer;
    xAudio2Buffer.AudioBytes = BufferSize;
    xAudio2Buffer.Flags = 0;
    xAudio2Buffer.LoopBegin = 0;
    xAudio2Buffer.LoopCount = 0;
    xAudio2Buffer.LoopLength = 0;
    xAudio2Buffer.pAudioData = pBuffer;
    xAudio2Buffer.pContext = (void*)new int[1];
    xAudio2Buffer.PlayBegin = 0;
    xAudio2Buffer.PlayLength = 0;
    return xAudio2Buffer;
}

bool XAudioPlay::setFormat(int bitsPerSample, int channels, int freq)
{
    if (m_waveFormat.wBitsPerSample == bitsPerSample 
        && m_waveFormat.nChannels == channels 
        && m_waveFormat.nSamplesPerSec == freq)
        return true;

    stopPlaying();

    if (m_sourceVoice)
        m_sourceVoice->DestroyVoice();

    m_sourceVoice = nullptr;

    m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat.wBitsPerSample = bitsPerSample;
    m_waveFormat.nChannels = channels;
    m_waveFormat.nSamplesPerSec = freq;
    m_waveFormat.nBlockAlign = bitsPerSample / 8 * channels;
    m_waveFormat.nAvgBytesPerSec = m_waveFormat.nBlockAlign * freq;
    m_waveFormat.cbSize = 0;

    HRESULT lRet;
    lRet = m_XAudio2->CreateSourceVoice(&m_sourceVoice,
        &m_waveFormat, 
        0,
        XAUDIO2_DEFAULT_FREQ_RATIO,
        &m_voiceCallBack,
        nullptr,
        nullptr);
     
    if (FAILED(lRet))
        return false;

    return true;
}

void XAudioPlay::readData(unsigned char *buffer, int length)
{
    int readPos = 0;
    bool isFirstSubmit = true;
    while (length > readPos)
    {
        while (m_unProcessedBufferCount + m_unReadingBufferCount >= m_maxBufferCount - 1)
        {
            if (isFirstSubmit)
            {
                isFirstSubmit = false;
                m_voiceCallBack.OnBufferEnd(nullptr);
            }
            WaitForSingleObject(m_voiceCallBack.m_hBufferEndEvent, INFINITE);
        }
        if (m_streamingBufferSize > length - readPos)
        {
            memcpy_s(m_audioData[m_writingBufferNumber], m_streamingBufferSize, buffer + readPos, length - readPos);
            memset(m_audioData[m_writingBufferNumber] + length - readPos, 0, m_streamingBufferSize - (length - readPos));
            break;
        }
        if (m_streamingBufferSize <= length - readPos)
        {
            memcpy_s(m_audioData[m_writingBufferNumber], m_streamingBufferSize, buffer + readPos, m_streamingBufferSize);
            readPos += m_streamingBufferSize;
        }
        m_writingBufferNumber = (1 + m_writingBufferNumber) % m_maxBufferCount;
        ++m_unReadingBufferCount;
    }
}