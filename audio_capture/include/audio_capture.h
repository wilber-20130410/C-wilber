/*
名称:audio_capture.h
作者:wilber-20130410
版权: © 2025~2026 wilber-20130410
版本:1.0.0[140200103150101](正式版)
日期:2026.1.3
留言:
1.本代码仅供学习交流使用,请勿用于商业用途。
2.本代码参考了网络上部分代码,在此表示感谢。
3.本人推荐使用Visual Studio Code作为IDE(集成开发环境)。
4.如果您有建议、发现了Bug、问题或者您进行优化后的代码,欢迎向本人邮箱xuwb0410@163.com发送邮件,本人将在21天内进行回复。
5.本代码适用于Windows系统。
6.以上留言不分先后。
*/

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <vector>
#include <functional>
#include <atomic>
#include <thread>

#ifdef _WIN32
    #ifdef AUDIO_CAPTURE_EXPORTS
        #define AUDIO_CAPTURE_API __declspec(dllexport)
    #else
        #define AUDIO_CAPTURE_API __declspec(dllimport)
    #endif
#else
    #define AUDIO_CAPTURE_API
#endif

#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <functiondiscoverykeys.h>
#endif

class AUDIO_CAPTURE_API AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();
    bool initialize();
    bool startCapture();
    void stopCapture();
    void setAudioCallback(std::function<void(const std::vector<float>&)> callback);
    int getSampLeRate() const { return sampleRate; }
    bool isRunning() const { return isCapturing; }

private:
    void captureThread();

#ifdef _WIN32
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* audioDevice =nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    WAVEFORMATEX* waveFormat = nullptr;
#endif

    std::atomic<bool> isCapturing;
    std::thread captureThreadHandle;
    std::function<void(const std::vector<float>&)> audioCallback;
    int sampleRate = 44100;
    int channels = 2;
};

#endif