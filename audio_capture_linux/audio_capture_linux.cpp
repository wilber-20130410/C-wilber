#include "audio_capture_linux.h"
#include <iostream>
#include <cstring>
#include <memory>
#include <cmath>

// PulseAudio包含
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

// ALSA包含
#include <alsa/asoundlib.h>

struct AudioCapture::PulseAudioData {
    pa_simple* simple = nullptr;
    pa_mainloop* mainloop = nullptr;
    pa_context* context = nullptr;
    pa_stream* stream = nullptr;
    
    ~PulseAudioData() {
        if (simple) {
            pa_simple_free(simple);
            simple = nullptr;
        }
        if (stream) {
            pa_stream_unref(stream);
            stream = nullptr;
        }
        if (context) {
            pa_context_disconnect(context);
            pa_context_unref(context);
            context = nullptr;
        }
        if (mainloop) {
            pa_mainloop_free(mainloop);
            mainloop = nullptr;
        }
    }
};

AudioCapture::AudioCapture() {
    std::cout << "AudioCapture Linux版本创建" << std::endl;
}

AudioCapture::~AudioCapture() {
    stopCapture();
    cleanupResources();
}

std::string AudioCapture::getBackendName() const {
    switch (currentBackend) {
        case Backend::PULSEAUDIO: return "PulseAudio";
        case Backend::ALSA: return "ALSA";
        case Backend::JACK: return "JACK";
        case Backend::AUTO: return "Auto";
        default: return "Unknown";
    }
}
bool AudioCapture::initialize(Backend preferredBackend) {
    if (isInitializedFlag) {
        std::cout << "已经初始化" << std::endl;
        return true;
    }
    currentBackend = preferredBackend;
    if (preferredBackend == Backend::AUTO || preferredBackend == Backend::PULSEAUDIO) {
        if (initializePulseAudio()) {
            std::cout << "使用PulseAudio后端" << std::endl;
            currentBackend = Backend::PULSEAUDIO;
            isInitializedFlag = true;
            return true;
        }
    }
    if (preferredBackend == Backend::AUTO || preferredBackend == Backend::ALSA) {
        if (initializeALSA()) {
            std::cout << "使用ALSA后端" << std::endl;
            currentBackend = Backend::ALSA;
            isInitializedFlag = true;
            return true;
        }
    }
    std::cerr << "所有音频后端初始化失败" << std::endl;
    return false;
}

bool AudioCapture::initializePulseAudio() {
    try {
        pulseData = std::make_unique<PulseAudioData>();
        // 使用pa_simple进行简单的音频捕获
        static const pa_sample_spec sampleSpec = {
            .format = PA_SAMPLE_S16LE,
            .rate = static_cast<uint32_t>(sampleRate),
            .channels = static_cast<uint8_t>(channels)
        };
        // 捕获监视源（系统输出）
        pulseData->simple = pa_simple_new(
            nullptr,                    // 使用默认服务器
            "AudioVisualizer",          // 应用名称
            PA_STREAM_RECORD,           // 方向：录音
            nullptr,                    // 使用默认设备
            "record",                   // 流描述
            &sampleSpec,                // 采样规格
            nullptr,                    // 无通道映射
            nullptr,                    // 无缓冲区属性
            nullptr                     // 无错误指针
        );
        if (!pulseData->simple) {
            std::cerr << "无法创建PulseAudio简单连接" << std::endl;
            pulseData.reset();
            return false;
        }
        std::cout << "PulseAudio初始化成功" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "PulseAudio初始化异常: " << e.what() << std::endl;
        pulseData.reset();
        return false;
    }
}

bool AudioCapture::initializeALSA() {
    int err;
    snd_pcm_hw_params_t* params;
    // 尝试打开默认设备进行捕获
    err = snd_pcm_open(reinterpret_cast<snd_pcm_t**>(&alsaHandle), "default", 
                      SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        std::cerr << "无法打开ALSA设备: " << snd_strerror(err) << std::endl;
        return false;
    }
    // 分配硬件参数对象
    snd_pcm_hw_params_alloca(&params);
    // 填充默认参数
    err = snd_pcm_hw_params_any(reinterpret_cast<snd_pcm_t*>(alsaHandle), params);
    if (err < 0) {
        std::cerr << "无法初始化ALSA硬件参数: " << snd_strerror(err) << std::endl;
        cleanupALSA();
        return false;
    }
    // 设置交错访问
    err = snd_pcm_hw_params_set_access(reinterpret_cast<snd_pcm_t*>(alsaHandle), params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        std::cerr << "无法设置ALSA访问类型: " << snd_strerror(err) << std::endl;
        cleanupALSA();
        return false;
    }
    // 设置采样格式为16位有符号小端
    err = snd_pcm_hw_params_set_format(reinterpret_cast<snd_pcm_t*>(alsaHandle), params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        std::cerr << "无法设置ALSA采样格式: " << snd_strerror(err) << std::endl;
        cleanupALSA();
        return false;
    }
    // 设置采样率
    unsigned int actualRate = sampleRate;
    err = snd_pcm_hw_params_set_rate_near(reinterpret_cast<snd_pcm_t*>(alsaHandle), 
                                         params, &actualRate, 0);
    if (err < 0) {
        std::cerr << "无法设置ALSA采样率: " << snd_strerror(err) << std::endl;
        cleanupALSA();
        return false;
    }
    if (actualRate != static_cast<unsigned int>(sampleRate)) {
        std::cout << "ALSA: 采样率调整为 " << actualRate << " Hz" << std::endl;
        sampleRate = actualRate;
    }
    // 设置声道数
    err = snd_pcm_hw_params_set_channels(reinterpret_cast<snd_pcm_t*>(alsaHandle), 
                                        params, channels);
    if (err < 0) {
        std::cerr << "无法设置ALSA声道数: " << snd_strerror(err) << std::endl;
        cleanupALSA();
        return false;
    }
    // 应用参数
    err = snd_pcm_hw_params(reinterpret_cast<snd_pcm_t*>(alsaHandle), params);
    if (err < 0) {
        std::cerr << "无法应用ALSA硬件参数: " << snd_strerror(err) << std::endl;
        cleanupALSA();
        return false;
    }
    std::cout << "ALSA初始化成功" << std::endl;
    return true;
}

void AudioCapture::cleanupResources() {
    if (currentBackend == Backend::PULSEAUDIO) {
        cleanupPulseAudio();
    } else if (currentBackend == Backend::ALSA) {
        cleanupALSA();
    }
    isInitializedFlag = false;
    std::cout << "音频资源已清理" << std::endl;
}

void AudioCapture::cleanupPulseAudio() {
    pulseData.reset();
}

void AudioCapture::cleanupALSA() {
    if (alsaHandle) {
        snd_pcm_close(reinterpret_cast<snd_pcm_t*>(alsaHandle));
        alsaHandle = nullptr;
    }
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
    captureThreadHandle = std::thread(&AudioCapture::captureThread, this);
    std::cout << "音频捕获开始 (" << getBackendName() << ")" << std::endl;
    return true;
}

void AudioCapture::stopCapture() {
    if (!isCapturing) {
        return;
    }
    std::cout << "停止音频捕获..." << std::endl;
    isCapturing = false;
    if (captureThreadHandle.joinable()) {
        captureThreadHandle.join();
        std::cout << "捕获线程已停止" << std::endl;
    }
}

void AudioCapture::setAudioCallback(std::function<void(const std::vector<float>&)> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    audioCallback = std::move(callback);
}

void AudioCapture::captureThread() {
    if (currentBackend == Backend::PULSEAUDIO) {
        captureThreadPulseAudio();
    } else if (currentBackend == Backend::ALSA) {
        captureThreadALSA();
    }
}

void AudioCapture::captureThreadPulseAudio() {
#ifdef HAVE_PULSEAUDIO
    const size_t bufferSize = 4096;
    std::vector<int16_t> buffer(bufferSize);
    
    std::cout << "PulseAudio捕获线程开始" << std::endl;
    
    while (isCapturing && pulseData && pulseData->simple) {
        int error;
        
        // 从PulseAudio读取数据
        if (pa_simple_read(pulseData->simple, buffer.data(), 
                          buffer.size() * sizeof(int16_t), &error) < 0) {
            std::cerr << "PulseAudio读取错误: " << pa_strerror(error) << std::endl;
            break;
        }
        
        std::function<void(const std::vector<float>&)> callback;
        {
            std::lock_guard<std::mutex> lock(callbackMutex);
            callback = audioCallback;
        }
        
        if (callback) {
            std::vector<float> audioData;
            size_t sampleCount = buffer.size();
            audioData.reserve(sampleCount);
            
            // 转换为浮点数
            for (size_t i = 0; i < sampleCount; ++i) {
                audioData.push_back(buffer[i] / 32768.0f);
            }
            
            // 调用回调 - 回调函数内部会处理GIL
            try {
                callback(audioData);
            } catch (const std::exception& e) {
                std::cerr << "回调函数异常: " << e.what() << std::endl;
            }
        }
        
        // 控制捕获速率
        usleep(10000); // 10ms
    }
    
    std::cout << "PulseAudio捕获线程结束" << std::endl;
#endif
}

void AudioCapture::captureThreadALSA() {
#ifdef HAVE_ALSA
    const snd_pcm_uframes_t frames = 512;
    std::vector<int16_t> buffer(frames * channels);
    
    std::cout << "ALSA捕获线程开始" << std::endl;
    
    while (isCapturing && alsaHandle) {
        int err = snd_pcm_readi(reinterpret_cast<snd_pcm_t*>(alsaHandle), 
                               buffer.data(), frames);
        
        if (err == -EPIPE) {
            // 欠载，重新准备
            std::cout << "ALSA欠载，重新准备..." << std::endl;
            snd_pcm_prepare(reinterpret_cast<snd_pcm_t*>(alsaHandle));
            continue;
        } else if (err < 0) {
            std::cerr << "ALSA读取错误: " << snd_strerror(err) << std::endl;
            break;
        } else if (err > 0) {
            std::function<void(const std::vector<float>&)> callback;
            {
                std::lock_guard<std::mutex> lock(callbackMutex);
                callback = audioCallback;
            }
            
            if (callback) {
                std::vector<float> audioData;
                size_t sampleCount = err * channels;
                audioData.reserve(sampleCount);
                
                for (size_t i = 0; i < sampleCount; ++i) {
                    audioData.push_back(buffer[i] / 32768.0f);
                }
                
                // 调用回调 - 回调函数内部会处理GIL
                try {
                    callback(audioData);
                } catch (const std::exception& e) {
                    std::cerr << "回调函数异常: " << e.what() << std::endl;
                }
            }
        }
        
        usleep(10000); // 10ms
    }
    
    std::cout << "ALSA捕获线程结束" << std::endl;
#endif
}