#ifndef AUDIO_CAPTURE_LINUX_H
#define AUDIO_CAPTURE_LINUX_H

#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <memory>

class AudioCapture {
public:
    // 将枚举声明为公共的
    enum class Backend {
        AUTO = 0,
        PULSEAUDIO = 1,
        ALSA = 2,
        JACK = 3
    };
    
    AudioCapture();
    ~AudioCapture();
    
    AudioCapture(const AudioCapture&) = delete;
    AudioCapture& operator=(const AudioCapture&) = delete;
    AudioCapture(AudioCapture&&) = delete;
    AudioCapture& operator=(AudioCapture&&) = delete;
    
    bool initialize(Backend preferredBackend = Backend::AUTO);
    bool startCapture();
    void stopCapture();
    
    void setAudioCallback(std::function<void(const std::vector<float>&)> callback);
    
    int getSampleRate() const { return sampleRate; }
    int getChannels() const { return channels; }
    bool isRunning() const { return isCapturing; }
    bool isInitialized() const { return isInitializedFlag; }
    
    std::string getBackendName() const;

private:
    void captureThread();
    void cleanupResources();
    
    // PulseAudio相关
    struct PulseAudioData;
    std::unique_ptr<PulseAudioData> pulseData;
    
    // ALSA相关
    void* alsaHandle = nullptr;
    
    // 通用
    std::atomic<bool> isCapturing{false};
    std::atomic<bool> isInitializedFlag{false};
    std::thread captureThreadHandle;
    
    std::function<void(const std::vector<float>&)> audioCallback;
    mutable std::mutex callbackMutex;
    
    int sampleRate = 44100;
    int channels = 2;
    Backend currentBackend = Backend::AUTO;
    
    bool initializePulseAudio();
    bool initializeALSA();
    void captureThreadPulseAudio();
    void captureThreadALSA();
    void cleanupPulseAudio();
    void cleanupALSA();
};

#endif