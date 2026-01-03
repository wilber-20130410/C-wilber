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