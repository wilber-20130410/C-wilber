/*
名称:audio_capture.cpp
作者:wilber-20130410
版权: © 2025~2026 wilber-20130410
版本:1.0.1[140200228185201](正式版)
日期:2026.2.28
留言:
1.本代码仅供学习交流使用,请勿用于商业用途。
2.本代码参考了网络上部分代码,在此表示感谢。
3.本人推荐使用Visual Studio Code作为IDE(集成开发环境)。
4.如果您有建议、发现了Bug、问题或者您进行优化后的代码,欢迎向本人邮箱xuwb0410@163.com发送邮件,本人将在21天内进行回复。
5.本代码适用于所有操作系统。
6.以上留言不分先后。
*/

#include "audio_capture.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// ==================== 通用实现 ====================

AudioCapture::AudioCapture() {
    std::cout << "跨平台音频捕获库初始化" << std::endl;
}

AudioCapture::~AudioCapture() {
    stopCapture();
    cleanupResources();
}

std::string AudioCapture::getBackendName() const {
    switch (currentBackend) {
        case Backend::WASAPI: return "Windows WASAPI";
        case Backend::PULSEAUDIO: return "Linux PulseAudio";
        case Backend::ALSA: return "Linux ALSA";
        case Backend::COREAUDIO: return "macOS CoreAudio";
        case Backend::AUTO: return "Auto";
        default: return "Unknown";
    }
}

bool AudioCapture::initialize(Backend preferredBackend) {
    if (isInitializedFlag) {
        std::cout << "已经初始化，跳过重复初始化" << std::endl;
        return true;
    }
    
    requestedBackend = preferredBackend;
    bool initialized = false;
    
#ifdef AUDIO_PLATFORM_WINDOWS
    if (preferredBackend == Backend::AUTO || preferredBackend == Backend::WASAPI) {
        initialized = initializeWindows();
        if (initialized) currentBackend = Backend::WASAPI;
    }
#endif

#ifdef AUDIO_PLATFORM_LINUX
    if (!initialized && (preferredBackend == Backend::AUTO || 
                        preferredBackend == Backend::PULSEAUDIO || 
                        preferredBackend == Backend::ALSA)) {
        initialized = initializeLinux();
        // 无需手动设置currentBackend，initializeLinux()内部已同步
    }
#endif

#ifdef AUDIO_PLATFORM_MACOS
    if (!initialized && (preferredBackend == Backend::AUTO || preferredBackend == Backend::COREAUDIO)) {
        initialized = initializeMacOS();
        if (initialized) currentBackend = Backend::COREAUDIO;
    }
#endif

    if (initialized) {
        isInitializedFlag = true;
        std::cout << "初始化成功 - 后端: " << getBackendName()
                  << ", 采样率: " << format.sampleRate
                  << ", 声道数: " << format.channels << std::endl;
    } else {
        std::cerr << "所有音频后端初始化失败" << std::endl;
    }
    
    return initialized;
}

bool AudioCapture::startCapture() {
    if (!isInitializedFlag) {
        std::cerr << "音频捕获未初始化" << std::endl;
        return false;
    }
    
    if (isCapturing) {
        std::cout << "已经在捕获中" << std::endl;
        return true;
    }
    
    isCapturing = true;
    
    try {
        captureThreadHandle = std::thread(&AudioCapture::captureThread, this);
        std::cout << "音频捕获开始" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "启动捕获线程失败: " << e.what() << std::endl;
        isCapturing = false;
        return false;
    }
}

void AudioCapture::stopCapture() {
    if (!isCapturing) return;
    
    std::cout << "停止音频捕获..." << std::endl;
    isCapturing = false;
    
    if (captureThreadHandle.joinable()) {
        captureThreadHandle.join();
        std::cout << "捕获线程已停止" << std::endl;
    }
}

void AudioCapture::setAudioCallback(std::function<void(const std::vector<float>&, const AudioFormat&)> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    audioCallback = std::move(callback);
}

void AudioCapture::cleanupResources() {
#ifdef AUDIO_PLATFORM_WINDOWS
    cleanupWindows();
#endif
#ifdef AUDIO_PLATFORM_LINUX
    cleanupLinux();
#endif
#ifdef AUDIO_PLATFORM_MACOS
    cleanupMacOS();
#endif
    
    isInitializedFlag = false;
    std::cout << "资源清理完成" << std::endl;
}

void AudioCapture::captureThread() {
#ifdef AUDIO_PLATFORM_WINDOWS
    if (currentBackend == Backend::WASAPI) {
        captureThreadWindows();
    }
#endif
#ifdef AUDIO_PLATFORM_LINUX
    if (currentBackend == Backend::PULSEAUDIO || currentBackend == Backend::ALSA) {
        captureThreadLinux();
    }
#endif
#ifdef AUDIO_PLATFORM_MACOS
    if (currentBackend == Backend::COREAUDIO) {
        captureThreadMacOS();
    }
#endif
}

// ==================== Windows 实现 ====================

#ifdef AUDIO_PLATFORM_WINDOWS

bool AudioCapture::initializeWindows() {
    std::cout << "初始化Windows WASAPI..." << std::endl;
    
    winData = std::make_unique<WindowsData>();
    HRESULT hr;
    
    // 初始化COM
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "COM初始化失败" << std::endl;
        return false;
    }
    winData->comInitialized = true;
    
    // 创建设备枚举器
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                         CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                         (void**)&winData->deviceEnumerator);
    if (FAILED(hr)) {
        std::cerr << "创建设备枚举器失败" << std::endl;
        cleanupWindows();
        return false;
    }
    
    // 获取默认音频渲染设备（系统输出）
    hr = winData->deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &winData->audioDevice);
    if (FAILED(hr)) {
        std::cerr << "获取默认音频设备失败" << std::endl;
        cleanupWindows();
        return false;
    }
    
    // 激活音频客户端
    hr = winData->audioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                                        nullptr, (void**)&winData->audioClient);
    if (FAILED(hr)) {
        std::cerr << "激活音频客户端失败" << std::endl;
        cleanupWindows();
        return false;
    }
    
    // 获取混音格式
    hr = winData->audioClient->GetMixFormat(&winData->waveFormat);
    if (FAILED(hr)) {
        std::cerr << "获取音频格式失败" << std::endl;
        cleanupWindows();
        return false;
    }
    
    format.sampleRate = winData->waveFormat->nSamplesPerSec;
    format.channels = winData->waveFormat->nChannels;
    format.bitsPerSample = winData->waveFormat->wBitsPerSample;
    
    return true;
}

void AudioCapture::cleanupWindows() {
    if (!winData) return;
    
    if (winData->captureClient) {
        winData->captureClient->Release();
        winData->captureClient = nullptr;
    }
    
    if (winData->audioClient) {
        winData->audioClient->Release();
        winData->audioClient = nullptr;
    }
    
    if (winData->audioDevice) {
        winData->audioDevice->Release();
        winData->audioDevice = nullptr;
    }
    
    if (winData->deviceEnumerator) {
        winData->deviceEnumerator->Release();
        winData->deviceEnumerator = nullptr;
    }
    
    if (winData->waveFormat) {
        CoTaskMemFree(winData->waveFormat);
        winData->waveFormat = nullptr;
    }
    
    if (winData->comInitialized) {
        CoUninitialize();
        winData->comInitialized = false;
    }
}

void AudioCapture::captureThreadWindows() {
    if (!winData || !winData->audioClient) return;
    
    HRESULT hr;
    
    // 初始化音频客户端用于循环捕获
    hr = winData->audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0, 0, winData->waveFormat, nullptr);
    
    if (FAILED(hr)) {
        std::cerr << "初始化音频客户端失败" << std::endl;
        return;
    }
    
    // 获取捕获客户端
    hr = winData->audioClient->GetService(__uuidof(IAudioCaptureClient),
                                          (void**)&winData->captureClient);
    if (FAILED(hr)) {
        std::cerr << "获取捕获客户端失败" << std::endl;
        return;
    }
    
    // 开始捕获
    hr = winData->audioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "开始捕获失败" << std::endl;
        return;
    }
    
    std::cout << "Windows捕获线程开始" << std::endl;
    
    UINT32 packetLength = 0;
    BYTE* data = nullptr;
    UINT32 numFramesAvailable = 0;
    DWORD flags = 0;
    
    while (isCapturing) {
        Sleep(10); // 10ms等待
        
        hr = winData->captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) continue;
        
        while (packetLength > 0) {
            hr = winData->captureClient->GetBuffer(&data, &numFramesAvailable,
                                                   &flags, nullptr, nullptr);
            
            if (SUCCEEDED(hr) && data && !(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                std::function<void(const std::vector<float>&, const AudioFormat&)> callback;
                {
                    std::lock_guard<std::mutex> lock(callbackMutex);
                    callback = audioCallback;
                }
                
                if (callback) {
                    std::vector<float> audioData;
                    size_t sampleCount = numFramesAvailable * format.channels;
                    
                    if (winData->waveFormat->wBitsPerSample == 16) {
                        int16_t* samples = reinterpret_cast<int16_t*>(data);
                        audioData.reserve(sampleCount);
                        for (size_t i = 0; i < sampleCount; ++i) {
                            audioData.push_back(samples[i] / 32768.0f);
                        }
                    } else if (winData->waveFormat->wBitsPerSample == 32) {
                        float* samples = reinterpret_cast<float*>(data);
                        audioData.assign(samples, samples + sampleCount);
                    }
                    
                    if (!audioData.empty()) {
                        try {
                            callback(audioData, format);
                        } catch (const std::exception& e) {
                            std::cerr << "回调异常: " << e.what() << std::endl;
                        }
                    }
                }
                
                winData->captureClient->ReleaseBuffer(numFramesAvailable);
            }
            
            hr = winData->captureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) break;
        }
    }
    
    winData->audioClient->Stop();
    std::cout << "Windows捕获线程结束" << std::endl;
}

std::vector<std::string> AudioCapture::getWindowsDevices() {
    std::vector<std::string> devices;
    // Windows设备枚举实现
    return devices;
}

#endif // AUDIO_PLATFORM_WINDOWS

// ==================== Linux 实现 ====================

#ifdef AUDIO_PLATFORM_LINUX

bool AudioCapture::initializeLinux() {
    std::cout << "初始化Linux音频..." << std::endl;
    
    linuxData = std::make_unique<LinuxData>();
    bool pulseInitSuccess = false;
    bool alsaInitSuccess = false;

    // ========== 修复1：检测PulseAudio服务是否运行 ==========
    bool pulseServiceRunning = false;
    FILE* pipe = popen("pgrep -x pulseaudio > /dev/null", "r");
    if (pipe) {
        int ret = pclose(pipe);
        pulseServiceRunning = (ret == 0); // 0表示进程存在
    }
    if (!pulseServiceRunning) {
        std::cerr << "PulseAudio服务未运行，直接尝试ALSA" << std::endl;
        linuxData->usePulseAudio = false;
    }

    // 尝试使用PulseAudio（仅当请求后端为AUTO/PULSEAUDIO且服务运行）
    if ((requestedBackend == Backend::AUTO || requestedBackend == Backend::PULSEAUDIO) && pulseServiceRunning) {
        linuxData->usePulseAudio = true;
        // 创建PulseAudio主循环
        linuxData->pulseThreadedMainloop = pa_threaded_mainloop_new();
        if (!linuxData->pulseThreadedMainloop) {
            std::cerr << "无法创建PulseAudio主循环，尝试ALSA" << std::endl;
            linuxData->usePulseAudio = false;
        } else {
            pa_threaded_mainloop_lock(linuxData->pulseThreadedMainloop);
            linuxData->pulseContext = pa_context_new(
                pa_threaded_mainloop_get_api(linuxData->pulseThreadedMainloop),
                "AudioCapture");
            if (!linuxData->pulseContext) {
                std::cerr << "无法创建PulseAudio上下文，尝试ALSA" << std::endl;
                pa_threaded_mainloop_unlock(linuxData->pulseThreadedMainloop);
                pa_threaded_mainloop_free(linuxData->pulseThreadedMainloop);
                linuxData->pulseThreadedMainloop = nullptr;
                linuxData->usePulseAudio = false;
            } else {
                // 设置上下文状态回调
                pa_context_set_state_callback(linuxData->pulseContext,
                    [](pa_context* c, void* userdata) {
                        auto* data = static_cast<LinuxData*>(userdata);
                        pa_threaded_mainloop_signal(data->pulseThreadedMainloop, 0);
                    }, linuxData.get());

                // ========== 修复2：添加PulseAudio连接超时参数 ==========
                pa_context_flags_t flags = PA_CONTEXT_NOFLAGS;
                pa_context_connect(linuxData->pulseContext, nullptr, flags, nullptr);

                pa_threaded_mainloop_unlock(linuxData->pulseThreadedMainloop);

                // ========== 修复3：兼容版带超时的上下文等待（5秒） ==========
                const int MAX_WAIT_MS = 5000; // 最大等待5秒
                const int CHECK_INTERVAL_MS = 100; // 每次检查间隔100ms
                int waitTime = 0;
                bool contextReady = false;

                pa_threaded_mainloop_lock(linuxData->pulseThreadedMainloop);
                while (waitTime < MAX_WAIT_MS) {
                    pa_context_state_t state = pa_context_get_state(linuxData->pulseContext);
                    // 重检状态，避免信号误触发
                    state = pa_context_get_state(linuxData->pulseContext);
                    if (state == PA_CONTEXT_READY) {
                        contextReady = true;
                        pulseInitSuccess = true;
                        break;
                    }
                    if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
                        std::cerr << "PulseAudio上下文连接失败" << std::endl;
                        pulseInitSuccess = false;
                        break;
                    }
                    // 兼容方案：使用usleep+非阻塞等待，替代不存在的wait_timeout
                    pa_threaded_mainloop_unlock(linuxData->pulseThreadedMainloop);
                    usleep(CHECK_INTERVAL_MS * 1000); // 等待100ms
                    pa_threaded_mainloop_lock(linuxData->pulseThreadedMainloop);
                    waitTime += CHECK_INTERVAL_MS;
                }
                // 超时判定
                if (waitTime >= MAX_WAIT_MS && !contextReady) {
                    std::cerr << "PulseAudio初始化超时（5秒），尝试ALSA" << std::endl;
                    pulseInitSuccess = false;
                    // 强制清理PulseAudio资源
                    pa_context_disconnect(linuxData->pulseContext);
                    pa_context_unref(linuxData->pulseContext);
                    linuxData->pulseContext = nullptr;
                }
                pa_threaded_mainloop_unlock(linuxData->pulseThreadedMainloop);

                if (pulseInitSuccess) {
                    format.sampleRate = 44100;
                    format.channels = 2;
                    format.bitsPerSample = 16;
                    currentBackend = Backend::PULSEAUDIO;
                    std::cout << "PulseAudio初始化成功" << std::endl;
                    return true;
                }
            }
        }
    }

    // 尝试ALSA（仅当请求后端为AUTO/ALSA，且PulseAudio失败/未请求/服务未运行）
    if ((requestedBackend == Backend::AUTO || requestedBackend == Backend::ALSA) && !pulseInitSuccess) {
        std::cout << "尝试使用ALSA..." << std::endl;
        linuxData->usePulseAudio = false;

        // 适配自定义设备名（可选功能）
        const char* alsaDevice = "default";
        if (!selectedDevice.empty() && selectedDevice.find("hw:") == 0) {
            alsaDevice = selectedDevice.c_str();
        }

        // 打开ALSA捕获设备
        int err = snd_pcm_open(&linuxData->alsaHandle, alsaDevice,
                               SND_PCM_STREAM_CAPTURE, 0);
        if (err < 0) {
            std::cerr << "无法打开ALSA设备: " << snd_strerror(err) << std::endl;
            alsaInitSuccess = false;
        } else {
            snd_pcm_hw_params_alloca(&linuxData->alsaParams);
            err = snd_pcm_hw_params_any(linuxData->alsaHandle, linuxData->alsaParams);
            if (err < 0) {
                std::cerr << "无法初始化ALSA参数: " << snd_strerror(err) << std::endl;
                alsaInitSuccess = false;
            } else {
                // 批量设置ALSA参数，减少错误判定
                err = 0;
                err |= snd_pcm_hw_params_set_access(linuxData->alsaHandle, linuxData->alsaParams, SND_PCM_ACCESS_RW_INTERLEAVED);
                err |= snd_pcm_hw_params_set_format(linuxData->alsaHandle, linuxData->alsaParams, SND_PCM_FORMAT_S16_LE);
                unsigned int actualRate = format.sampleRate;
                err |= snd_pcm_hw_params_set_rate_near(linuxData->alsaHandle, linuxData->alsaParams, &actualRate, 0);
                err |= snd_pcm_hw_params_set_channels(linuxData->alsaHandle, linuxData->alsaParams, format.channels);

                if (err < 0) {
                    std::cerr << "无法设置ALSA参数: " << snd_strerror(err) << std::endl;
                    alsaInitSuccess = false;
                } else {
                    // 应用ALSA参数
                    err = snd_pcm_hw_params(linuxData->alsaHandle, linuxData->alsaParams);
                    if (err < 0) {
                        std::cerr << "无法应用ALSA参数: " << snd_strerror(err) << std::endl;
                        alsaInitSuccess = false;
                    } else {
                        // 更新实际生效的音频格式
                        format.sampleRate = actualRate;
                        alsaInitSuccess = true;
                        currentBackend = Backend::ALSA;
                        std::cout << "ALSA初始化成功，实际采样率: " << actualRate << "Hz" << std::endl;
                    }
                }
            }
        }

        if (alsaInitSuccess) {
            return true;
        }
    }

    // 所有后端均失败，清理资源
    std::cerr << "PulseAudio和ALSA均初始化失败" << std::endl;
    cleanupLinux();
    return false;
}

void AudioCapture::cleanupLinux() {
    if (!linuxData) return;
    
    if (linuxData->usePulseAudio) {
        if (linuxData->pulseStream) {
            pa_stream_disconnect(linuxData->pulseStream);
            pa_stream_unref(linuxData->pulseStream);
            linuxData->pulseStream = nullptr;
        }
        if (linuxData->pulseContext) {
            if (pa_context_get_state(linuxData->pulseContext) != PA_CONTEXT_TERMINATED) {
                pa_context_disconnect(linuxData->pulseContext);
            }
            pa_context_unref(linuxData->pulseContext);
            linuxData->pulseContext = nullptr;
        }
        if (linuxData->pulseThreadedMainloop) {
            // 先解锁再释放，避免死锁
            pa_threaded_mainloop_unlock(linuxData->pulseThreadedMainloop);
            pa_threaded_mainloop_stop(linuxData->pulseThreadedMainloop);
            pa_threaded_mainloop_free(linuxData->pulseThreadedMainloop);
            linuxData->pulseThreadedMainloop = nullptr;
        }
    } else {
        if (linuxData->alsaHandle) {
            snd_pcm_drop(linuxData->alsaHandle);
            snd_pcm_close(linuxData->alsaHandle);
            linuxData->alsaHandle = nullptr;
        }
    }
}

void AudioCapture::captureThreadLinux() {
    if (!linuxData) return;
    
    if (linuxData->usePulseAudio) {
        // PulseAudio捕获实现
        std::cout << "Linux PulseAudio捕获线程开始" << std::endl;
        
        const size_t bufferSize = 4096;
        std::vector<int16_t> buffer(bufferSize);
        
        pa_sample_spec sampleSpec = {
            .format = PA_SAMPLE_S16LE,
            .rate = static_cast<uint32_t>(format.sampleRate),
            .channels = static_cast<uint8_t>(format.channels)
        };
        
        pa_simple* simple = pa_simple_new(
            nullptr, "AudioCapture", PA_STREAM_RECORD,
            nullptr, "record", &sampleSpec, nullptr, nullptr, nullptr);
        
        if (!simple) {
            std::cerr << "无法创建PulseAudio简单连接" << std::endl;
            return;
        }
        
        while (isCapturing) {
            int error;
            if (pa_simple_read(simple, buffer.data(),
                            buffer.size() * sizeof(int16_t), &error) < 0) {
                std::cerr << "PulseAudio读取错误: " << pa_strerror(error) << "，停止捕获" << std::endl;
                pa_simple_free(simple);
                return; // 优雅退出，避免死循环
            }
        
            // 过滤全零空数据
            bool isEmpty = true;
            for (int16_t s : buffer) {
                if (s != 0) {
                    isEmpty = false;
                    break;
                }
            }
            if (isEmpty) {
                usleep(10000);
                continue;
            }
        
            std::function<void(const std::vector<float>&, const AudioFormat&)> callback;
            {
                std::lock_guard<std::mutex> lock(callbackMutex);
                callback = audioCallback;
            }
        
            if (callback) {
                std::vector<float> audioData;
                audioData.reserve(buffer.size());
                for (int16_t sample : buffer) {
                    audioData.push_back(sample / 32768.0f);
                }
        
                try {
                    callback(audioData, format);
                } catch (const std::exception& e) {
                    std::cerr << "回调异常: " << e.what() << std::endl;
                }
            }
        
            usleep(10000); // 10ms
        }
        
        pa_simple_free(simple);
        
    } else {
        // ALSA捕获实现
        std::cout << "Linux ALSA捕获线程开始" << std::endl;
        
        const snd_pcm_uframes_t frames = 512;
        std::vector<int16_t> buffer(frames * format.channels);
        
        while (isCapturing && linuxData->alsaHandle) {
            int err = snd_pcm_readi(linuxData->alsaHandle, buffer.data(), frames);
            if (err == -EPIPE) {
                snd_pcm_prepare(linuxData->alsaHandle);
                std::cerr << "ALSA欠载，重新准备设备" << std::endl;
                continue;
            } else if (err == -EIO) {
                std::cerr << "ALSA设备断开，停止捕获" << std::endl;
                break;
            } else if (err < 0) {
                err = snd_pcm_recover(linuxData->alsaHandle, err, 0);
                if (err < 0) {
                    std::cerr << "ALSA读取错误: " << snd_strerror(err) << "，停止捕获" << std::endl;
                    break;
                }
                continue;
            } else if (err > 0) {
                std::function<void(const std::vector<float>&, const AudioFormat&)> callback;
                {
                    std::lock_guard<std::mutex> lock(callbackMutex);
                    callback = audioCallback;
                }
                
                if (callback) {
                    std::vector<float> audioData;
                    size_t sampleCount = err * format.channels;
                    audioData.reserve(sampleCount);
                    
                    for (size_t i = 0; i < sampleCount; ++i) {
                        audioData.push_back(buffer[i] / 32768.0f);
                    }
                    
                    try {
                        callback(audioData, format);
                    } catch (const std::exception& e) {
                        std::cerr << "回调异常: " << e.what() << std::endl;
                    }
                }
            }
            
            usleep(10000);
        }
    }
    
    std::cout << "Linux捕获线程结束" << std::endl;
}

std::vector<std::string> AudioCapture::getLinuxDevices() {
    std::vector<std::string> devices;
    snd_ctl_t *handle = nullptr;
    snd_ctl_card_info_t *card_info = nullptr;
    snd_pcm_info_t *pcm_info = nullptr;
    int card = -1, dev = 0;
    int err;

    // 初始化ALSA信息结构体
    snd_ctl_card_info_alloca(&card_info);
    snd_pcm_info_alloca(&pcm_info);

    // 遍历所有声卡
    while (snd_card_next(&card) >= 0 && card >= 0) {
        char card_name[32];
        snprintf(card_name, sizeof(card_name), "hw:%d", card);
        // 打开声卡控制接口
        if ((err = snd_ctl_open(&handle, card_name, 0)) < 0) {
            continue;
        }
        // 获取声卡信息
        if ((err = snd_ctl_card_info(handle, card_info)) < 0) {
            snd_ctl_close(handle);
            continue;
        }

        // 遍历声卡下的所有PCM设备（仅捕获设备）
        dev = -1;
        while (snd_ctl_pcm_next_device(handle, &dev) >= 0 && dev >= 0) {
            snd_pcm_info_set_device(pcm_info, dev);
            snd_pcm_info_set_subdevice(pcm_info, 0);
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE);
            if ((err = snd_ctl_pcm_info(handle, pcm_info)) < 0) {
                continue;
            }
            // 拼接设备名称：声卡名 - 设备名 (hw:X,Y)
            char device_name[128];
            snprintf(device_name, sizeof(device_name),
                     "%s - %s (hw:%d,%d)",
                     snd_ctl_card_info_get_name(card_info),
                     snd_pcm_info_get_name(pcm_info),
                     card, dev);
            devices.push_back(std::string(device_name));
        }
        snd_ctl_close(handle);
    }

    // 若未枚举到设备，添加默认设备占位
    if (devices.empty()) {
        devices.push_back("Default Audio Device (hw:0,0)");
    }
    return devices;
}

#endif // AUDIO_PLATFORM_LINUX

// ==================== macOS 实现 ====================

#ifdef AUDIO_PLATFORM_MACOS

bool AudioCapture::initializeMacOS() {
    std::cout << "初始化macOS CoreAudio..." << std::endl;
    // macOS初始化实现
    return false; // 暂未实现
}

void AudioCapture::cleanupMacOS() {
    // macOS清理实现
}

void AudioCapture::captureThreadMacOS() {
    // macOS捕获线程实现
}

std::vector<std::string> AudioCapture::getMacOSDevices() {
    std::vector<std::string> devices;
    return devices;
}

#endif // AUDIO_PLATFORM_MACOS

// ==================== 通用设备枚举 ====================

std::vector<std::string> AudioCapture::getInputDevices() {
    std::lock_guard<std::mutex> lock(deviceMutex);
    
    std::vector<std::string> devices;
    
#ifdef AUDIO_PLATFORM_WINDOWS
    devices = getWindowsDevices();
#elif defined(AUDIO_PLATFORM_LINUX)
    devices = getLinuxDevices();
#elif defined(AUDIO_PLATFORM_MACOS)
    devices = getMacOSDevices();
#endif
    
    return devices;
}

bool AudioCapture::setInputDevice(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(deviceMutex);
    selectedDevice = deviceName;
    // 提取hw:X,Y格式的设备号（从设备名中匹配）
    size_t hw_pos = deviceName.find("(hw:");
    if (hw_pos != std::string::npos) {
        size_t hw_end = deviceName.find(")", hw_pos);
        if (hw_end != std::string::npos) {
            selectedDevice = deviceName.substr(hw_pos, hw_end - hw_pos + 1);
        }
    }
    std::cout << "选择音频设备: " << selectedDevice << std::endl;
    return true;
}