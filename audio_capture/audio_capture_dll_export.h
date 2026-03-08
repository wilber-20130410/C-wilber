/*
名称:audio_capture_dll_export.h
作者:wilber-20130410
版权: © 2025~2026 wilber-20130410
版本:1.0.1[140200301144501](正式版)
日期:2026.2.28
留言:
1.本代码仅供学习交流使用,请勿用于商业用途。
2.本代码参考了网络上部分代码,在此表示感谢。
3.本人推荐使用Visual Studio Code作为IDE(集成开发环境)。
4.如果您有建议、发现了Bug、问题或者您进行优化后的代码,欢迎向本人邮箱xuwb0410@163.com发送邮件,本人将在21天内进行回复。
5.本代码适用于所有操作系统。
6.以上留言不分先后。
*/

#ifndef AUDIO_CAPTURE_DLL_EXPORT_H
#define AUDIO_CAPTURE_DLL_EXPORT_H

// 跨平台DLL导出/导入宏定义
#ifdef _WIN32
    // Windows平台显式导出/导入符号
    #ifdef AUDIO_CAPTURE_DLL_EXPORT
        // 编译DLL时：导出符号
        #define AUDIO_CAPTURE_API __declspec(dllexport)
    #else
        // 使用DLL时：导入符号
        #define AUDIO_CAPTURE_API __declspec(dllimport)
    #endif
#else
    // Linux/macOS平台无需显式宏，直接定义为空
    #define AUDIO_CAPTURE_API
#endif

// 音频回调函数类型定义
#include <vector>
#include <functional>

// 前置声明
struct AudioFormat;

/**
 * 音频捕获回调函数类型
 * @param data 音频采样数据（float类型，范围[-1.0, 1.0]）
 * @param format 音频格式信息（采样率、声道数、位深）
 */
using AudioCaptureCallback = std::function<void(const std::vector<float>& data, const AudioFormat& format)>;

/**
 * 音频格式结构体
 * 存储采样率、声道数、位深等核心信息
 */
struct AUDIO_CAPTURE_API AudioFormat
{
    // 默认构造：44100Hz/2声道/16位深（通用音频配置）
    AudioFormat() : sampleRate(44100), channels(2), bitsPerSample(16) {}
    AudioFormat(int sr, int ch, int bps) : sampleRate(sr), channels(ch), bitsPerSample(bps) {}

    int sampleRate;        // 采样率 (Hz)
    int channels;          // 声道数 (1=单声道, 2=立体声)
    int bitsPerSample;     // 位深 (8/16/24/32)
};

/**
 * 音频捕获后端枚举
 * 跨平台支持不同的音频驱动后端
 */
enum class AUDIO_CAPTURE_API Backend
{
    AUTO = 0,        // 自动选择最佳后端（推荐）
    WASAPI = 1,      // Windows WASAPI（低延迟，Windows专属）
    PULSEAUDIO = 2,  // Linux PulseAudio（桌面版Linux默认）
    ALSA = 3,        // Linux ALSA（底层驱动，服务器/虚拟机Linux）
    COREAUDIO = 4    // macOS CoreAudio（macOS专属）
};

/**
 * 核心音频捕获类（跨平台）
 * 提供音频初始化、捕获启停、设备管理、回调设置等功能
 */
class AUDIO_CAPTURE_API AudioCapture
{
public:
    // 构造/析构
    AudioCapture();
    virtual ~AudioCapture();

    /**
     * 初始化音频捕获
     * @param preferredBackend 首选后端，默认AUTO自动选择
     * @return 初始化成功返回true，失败返回false
     */
    bool initialize(Backend preferredBackend = Backend::AUTO);

    /**
     * 开始音频捕获
     * 启动后会持续通过回调函数推送音频数据
     * @return 启动成功返回true，失败返回false
     */
    bool startCapture();

    /**
     * 停止音频捕获
     * 暂停音频数据采集，释放临时资源
     */
    void stopCapture();

    /**
     * 设置音频数据回调函数
     * 捕获到音频数据后会异步调用该函数
     * @param callback 自定义回调函数
     */
    void setAudioCallback(const AudioCaptureCallback& callback);

    /**
     * 获取当前音频格式
     * @return 已初始化的音频格式信息
     */
    AudioFormat getFormat() const;

    /**
     * 获取采样率（快捷方法）
     * @return 采样率 (Hz)
     */
    int getSampleRate() const { return getFormat().sampleRate; }

    /**
     * 获取声道数（快捷方法）
     * @return 声道数
     */
    int getChannels() const { return getFormat().channels; }

    /**
     * 获取位深（快捷方法）
     * @return 位深
     */
    int getBitsPerSample() const { return getFormat().bitsPerSample; }

    /**
     * 检查是否正在捕获音频
     * @return 正在捕获返回true，否则false
     */
    bool isRunning() const;

    /**
     * 检查是否已完成初始化
     * @return 已初始化返回true，否则false
     */
    bool isInitialized() const;

    /**
     * 获取当前使用的音频后端
     * @return 当前后端枚举值
     */
    Backend getCurrentBackend() const;

    /**
     * 获取当前后端的名称（字符串）
     * @return 后端名称（如"WASAPI"、"ALSA"）
     */
    std::string getBackendName() const;

    /**
     * 获取可用的输入设备列表
     * @return 设备名称列表
     */
    std::vector<std::string> getInputDevices();

    /**
     * 设置指定的输入设备
     * @param deviceName 设备名称（从getInputDevices获取）
     * @return 设置成功返回true，失败返回false
     */
    bool setInputDevice(const std::string& deviceName);

    /**
     * 清理资源
     * 手动释放所有音频相关资源，析构时会自动调用
     */
    void cleanup();

private:
    // 屏蔽拷贝构造和赋值运算符（单实例资源管理）
    AudioCapture(const AudioCapture&) = delete;
    AudioCapture& operator=(const AudioCapture&) = delete;

    // 平台相关私有数据（使用PIMPL模式隐藏实现）
    struct Impl;
    Impl* pImpl;
};

// 全局辅助函数：获取后端枚举对应的字符串名称
AUDIO_CAPTURE_API std::string backendToString(Backend backend);
// 全局辅助函数：从字符串名称解析后端枚举
AUDIO_CAPTURE_API Backend stringToBackend(const std::string& name);

#endif // AUDIO_CAPTURE_DLL_EXPORT_H