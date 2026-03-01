/*
名称:py_audio_capture.cpp
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

#ifdef _MSC_VER
    #define ATTRIBUTE_VISIBILITY_DEFAULT
#else
    #define ATTRIBUTE_VISIBILITY_DEFAULT __attribute__((visibility("default")))
#endif

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "audio_capture.h"
#include <iostream>
#include <memory>
#include <thread>

namespace py = pybind11;

// 音频格式的Python包装
class PyAudioFormat {
public:
    PyAudioFormat(const AudioCapture::AudioFormat& fmt) : format(fmt) {}
    
    int getSampleRate() const { return format.sampleRate; }
    int getChannels() const { return format.channels; }
    int getBitsPerSample() const { return format.bitsPerSample; }
    
    std::string __repr__() const {
        return "<AudioFormat: " + std::to_string(format.sampleRate) + "Hz, " +
               std::to_string(format.channels) + "ch, " +
               std::to_string(format.bitsPerSample) + "bit>";
    }
    
private:
    AudioCapture::AudioFormat format;
};

// 主类包装
class PyAudioCapture {
private:
    AudioCapture* capture;

public:
    PyAudioCapture() : capture(new AudioCapture()) {
        std::cout << "PyAudioCapture 创建" << std::endl;
    }
    
    ~PyAudioCapture() {
        std::cout << "PyAudioCapture 销毁" << std::endl;
        if (capture) {
            capture->stopCapture();
            delete capture;
            capture = nullptr;
        }
    }
    
    bool initialize(int backend = 0) {
        return capture->initialize(static_cast<AudioCapture::Backend>(backend));
    }
    
    bool start_capture() {
        return capture->startCapture();
    }
    
    void stop_capture() {
        capture->stopCapture();
    }
    
    void set_audio_callback(py::function callback) {
        // 使用shared_ptr确保线程安全
        auto py_callback = std::make_shared<py::function>(std::move(callback));
        
        // 移除 GCC 特有的 __attribute__，使用标准 C++
        capture->setAudioCallback([py_callback](const std::vector<float>& data,
                                                const AudioCapture::AudioFormat& format) {
            // 异步执行Python回调
            std::thread([py_callback, data, format]() {
                // 获取GIL
                py::gil_scoped_acquire gil;
                try {
                    if (py_callback && *py_callback) {
                        PyAudioFormat pyFormat(format);
                        (*py_callback)(data, pyFormat);
                    }
                } catch (const py::error_already_set& e) {
                    std::cerr << "Python回调错误: ";
                    PyErr_Print();
                    PyErr_Clear();
                } catch (const std::exception& e) {
                    std::cerr << "回调异常: " << e.what() << std::endl;
                }
            }).detach(); // 分离线程，避免阻塞捕获线程
        });
    }
    
    py::object get_format() {
        auto fmt = capture->getFormat();
        return py::cast(PyAudioFormat(fmt));
    }
    
    bool is_running() const {
        return capture->isRunning();
    }
    
    bool is_initialized() const {
        return capture->isInitialized();
    }
    
    int get_current_backend() const {
        return static_cast<int>(capture->getCurrentBackend());
    }
    
    std::string get_backend_name() const {
        return capture->getBackendName();
    }
    
    std::vector<std::string> get_input_devices() {
        return capture->getInputDevices();
    }
    
    bool set_input_device(const std::string& deviceName) {
        return capture->setInputDevice(deviceName);
    }
};

PYBIND11_MODULE(audio_capture, m) {
    m.doc() = "跨平台音频捕获库 (支持Windows/Linux)";
    
    // 定义后端枚举
    py::enum_<AudioCapture::Backend>(m, "Backend")
        .value("AUTO", AudioCapture::Backend::AUTO)
        .value("WASAPI", AudioCapture::Backend::WASAPI)
        .value("PULSEAUDIO", AudioCapture::Backend::PULSEAUDIO)
        .value("ALSA", AudioCapture::Backend::ALSA)
        .value("COREAUDIO", AudioCapture::Backend::COREAUDIO)
        .export_values();
    
    // 音频格式类
    py::class_<PyAudioFormat>(m, "AudioFormat")
        .def_property_readonly("sample_rate", &PyAudioFormat::getSampleRate)
        .def_property_readonly("channels", &PyAudioFormat::getChannels)
        .def_property_readonly("bits_per_sample", &PyAudioFormat::getBitsPerSample)
        .def("__repr__", &PyAudioFormat::__repr__);
    
    // 主类
    py::class_<PyAudioCapture>(m, "AudioCapture")
        .def(py::init<>(), "创建音频捕获对象")
        .def("initialize", &PyAudioCapture::initialize,
             py::arg("backend") = static_cast<int>(AudioCapture::Backend::AUTO),
             "初始化音频捕获系统")
        .def("start_capture", &PyAudioCapture::start_capture,
             "开始音频捕获")
        .def("stop_capture", &PyAudioCapture::stop_capture,
             "停止音频捕获")
        .def("set_audio_callback", &PyAudioCapture::set_audio_callback,
             "设置音频数据回调函数\n"
             "回调函数接收两个参数: (data: List[float], format: AudioFormat)")
        .def_property_readonly("format", &PyAudioCapture::get_format,
             "获取当前音频格式")
        .def_property_readonly("is_running", &PyAudioCapture::is_running,
             "是否正在运行")
        .def_property_readonly("is_initialized", &PyAudioCapture::is_initialized,
             "是否已初始化")
        .def_property_readonly("current_backend", &PyAudioCapture::get_current_backend,
             "获取当前后端ID")
        .def_property_readonly("backend_name", &PyAudioCapture::get_backend_name,
             "获取当前后端名称")
        .def("get_input_devices", &PyAudioCapture::get_input_devices,
             "获取可用输入设备列表")
        .def("set_input_device", &PyAudioCapture::set_input_device,
             "设置输入设备")
        .def("__repr__", [](const PyAudioCapture& self) {
            return "<AudioCapture object>";
        });
    
    // 版本信息
    m.attr("__version__") = "1.0.1";
    
    // 平台信息
#ifdef _WIN32
    m.attr("__platform__") = "Windows";
#elif defined(__linux__)
    m.attr("__platform__") = "Linux";
#elif defined(__APPLE__)
    m.attr("__platform__") = "macOS";
#endif
}