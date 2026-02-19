/*
名称:audio_capture.h
作者:wilber-20130410
版权: © 2025~2026 wilber-20130410
版本:1.0.1[140200103150101](正式版)
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
#include <mutex>
#include <string>
#include <memory>
#include <iostream>

// 平台检测
#ifdef _WIN32
    #define AUDIO_PLATFORM_WINDOWS
    #include <windows.h>
    #include <mmdeviceapi.h>
    #include <audioclient.h>
    #include <functiondiscoverykeys.h>
#elif defined(__linux__)
    #define AUDIO_PLATFORM_LINUX
    #include <pulse/pulseaudio.h>
    #include <pulse/simple.h>
    #include <alsa/asoundlib.h>
#elif defined(__APPLE__)
    #define AUDIO_PLATFORM_MACOS
    #include <CoreAudio/CoreAudio.h>
    #include <AudioToolbox/AudioToolbox.h>
#endif

class AudioCapture {
public:
    // 可用后端类型
    enum class Backend {
        AUTO = 0,        // 自动选择
        WASAPI = 1,      // Windows: WASAPI
        PULSEAUDIO = 2,  // Linux: PulseAudio
        ALSA = 3,        // Linux: ALSA
        COREAUDIO = 4    // macOS: CoreAudio
    };
    
    // 音频格式
    struct AudioFormat {
        int sampleRate = 44100;
        int channels = 2;
        int bitsPerSample = 16;
    };
    
    AudioCapture();
    ~AudioCapture();
    
    // 禁用拷贝
    AudioCapture(const AudioCapture&) = delete;
    AudioCapture& operator=(const AudioCapture&) = delete;
    
    // 初始化
    bool initialize(Backend preferredBackend = Backend::AUTO);
    
    // 开始/停止捕获
    bool startCapture();
    void stopCapture();
    
    // 设置回调
    void setAudioCallback(std::function<void(const std::vector<float>&, const AudioFormat&)> callback);
    
    // 获取信息
    AudioFormat getFormat() const { return format; }
    bool isRunning() const { return isCapturing; }
    bool isInitialized() const { return isInitializedFlag; }
    Backend getCurrentBackend() const { return currentBackend; }
    std::string getBackendName() const;
    
    // 获取可用设备列表
    std::vector<std::string> getInputDevices();
    bool setInputDevice(const std::string& deviceName);

private:
    // 线程函数
    void captureThread();
    void cleanupResources();
    
    // 平台特定初始化
    bool initializeWindows();
    bool initializeLinux();
    bool initializeMacOS();
    
    // 平台特定捕获函数
    void captureThreadWindows();
    void captureThreadLinux();
    void captureThreadMacOS();
    
    // 平台特定清理
    void cleanupWindows();
    void cleanupLinux();
    void cleanupMacOS();
    
    // 平台特定设备枚举
    std::vector<std::string> getWindowsDevices();
    std::vector<std::string> getLinuxDevices();
    std::vector<std::string> getMacOSDevices();
    
    // 平台特定数据
#ifdef AUDIO_PLATFORM_WINDOWS
    struct WindowsData {
        IMMDeviceEnumerator* deviceEnumerator = nullptr;
        IMMDevice* audioDevice = nullptr;
        IAudioClient* audioClient = nullptr;
        IAudioCaptureClient* captureClient = nullptr;
        WAVEFORMATEX* waveFormat = nullptr;
        bool comInitialized = false;
        HANDLE captureEvent = nullptr;
    };
    std::unique_ptr<WindowsData> winData;
#endif

#ifdef AUDIO_PLATFORM_LINUX
    struct LinuxData {
        // PulseAudio
        pa_mainloop* pulseMainloop = nullptr;
        pa_context* pulseContext = nullptr;
        pa_stream* pulseStream = nullptr;
        pa_threaded_mainloop* pulseThreadedMainloop = nullptr;
        
        // ALSA
        snd_pcm_t* alsaHandle = nullptr;
        snd_pcm_hw_params_t* alsaParams = nullptr;
        
        // 通用
        bool usePulseAudio = true;
    };
    std::unique_ptr<LinuxData> linuxData;
#endif

#ifdef AUDIO_PLATFORM_MACOS
    struct MacOSData {
        AudioUnit audioUnit = nullptr;
        AudioBufferList* bufferList = nullptr;
        AudioDeviceID deviceID = 0;
    };
    std::unique_ptr<MacOSData> macData;
#endif

    // 通用数据
    std::atomic<bool> isCapturing{false};
    std::atomic<bool> isInitializedFlag{false};
    std::thread captureThreadHandle;
    
    std::function<void(const std::vector<float>&, const AudioFormat&)> audioCallback;
    mutable std::mutex callbackMutex;
    mutable std::mutex deviceMutex;
    
    AudioFormat format;
    Backend currentBackend = Backend::AUTO;
    Backend requestedBackend = Backend::AUTO;
    std::string selectedDevice;
};

#endif // AUDIO_CAPTURE_H