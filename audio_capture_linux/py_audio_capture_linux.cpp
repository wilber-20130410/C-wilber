#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "audio_capture_linux.h"
#include <iostream>
#include <memory>

namespace py = pybind11;

class PyAudioCapture {
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
    
    PyAudioCapture(const PyAudioCapture&) = delete;
    PyAudioCapture& operator=(const PyAudioCapture&) = delete;
    PyAudioCapture(PyAudioCapture&&) = delete;
    PyAudioCapture& operator=(PyAudioCapture&&) = delete;
    
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
        // 将Python函数包装到shared_ptr中，确保线程安全
        auto py_callback = std::make_shared<py::function>(callback);
        
        capture->setAudioCallback([py_callback](const std::vector<float>& data) {
            // 在音频线程中，我们需要获取GIL
            py::gil_scoped_acquire gil;
            try {
                if (py_callback && *py_callback) {
                    (*py_callback)(data);
                }
            } catch (const py::error_already_set& e) {
                std::cerr << "Python回调错误: ";
                PyErr_Print();
            } catch (const std::exception& e) {
                std::cerr << "C++回调错误: " << e.what() << std::endl;
            }
        });
    }
    
    int get_sample_rate() const {
        return capture->getSampleRate();
    }
    
    int get_channels() const {
        return capture->getChannels();
    }
    
    bool is_running() const {
        return capture->isRunning();
    }
    
    bool is_initialized() const {
        return capture->isInitialized();
    }
    
    std::string get_backend_name() const {
        return capture->getBackendName();
    }

private:
    AudioCapture* capture;
};

PYBIND11_MODULE(audio_capture, m) {
    m.doc() = "Linux音频捕获库 (支持PulseAudio和ALSA)";
    
    // 使用正确的方式定义枚举
    py::enum_<AudioCapture::Backend> backend_enum(m, "Backend", "音频后端类型");
    backend_enum.value("AUTO", AudioCapture::Backend::AUTO)
               .value("PULSEAUDIO", AudioCapture::Backend::PULSEAUDIO)
               .value("ALSA", AudioCapture::Backend::ALSA)
               .value("JACK", AudioCapture::Backend::JACK)
               .export_values();
    
    py::class_<PyAudioCapture>(m, "AudioCapture")
        .def(py::init<>(), "创建音频捕获对象")
        .def("initialize", &PyAudioCapture::initialize, 
             py::arg("backend") = AudioCapture::Backend::AUTO,
             "初始化音频捕获系统")
        .def("start_capture", &PyAudioCapture::start_capture, 
             "开始音频捕获")
        .def("stop_capture", &PyAudioCapture::stop_capture, 
             "停止音频捕获")
        .def("set_audio_callback", &PyAudioCapture::set_audio_callback, 
             "设置音频数据回调函数")
        .def("get_sample_rate", &PyAudioCapture::get_sample_rate, 
             "获取当前采样率")
        .def("get_channels", &PyAudioCapture::get_channels, 
             "获取声道数")
        .def("is_running", &PyAudioCapture::is_running, 
             "检查是否正在运行")
        .def("is_initialized", &PyAudioCapture::is_initialized, 
             "检查是否已初始化")
        .def("get_backend_name", &PyAudioCapture::get_backend_name, 
             "获取当前使用的后端名称")
        .def("__repr__", [](const PyAudioCapture& self) {
            return "<AudioCapture object>";
        });
    
    m.attr("__version__") = "1.0.0-linux";
}