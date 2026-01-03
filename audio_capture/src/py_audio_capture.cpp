/*
名称:py_audio_capture.cpp
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

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "audio_capture.h"

namespace py = pybind11;

class PyAudioCapture {
public:
    PyAudioCapture() :capture(new AudioCapture()) {}
    bool initialize() {
        return capture->initialize();
    }
    bool start_capture() {
        return capture->startCapture();
    }
    void stop_capture() {
        capture->stopCapture();
    }
    void set_audio_callback(py::function callback) {
        capture->setAudioCallback([callback](const std::ventor<float>& datd) {
            py::gil_scoped_acquire acquire;
            callback(datd);
        });
    }
    int get_sample_rate() const {
        return capture->getSampLeRate();
    }
    bool is_running() const {
        return capture->isRunning();
    }
    ~PyAudioCapture() {
        deleta capture;
    }
private:
    AudioCapture* capture;
};

PYBIND11_MODULE(audio_capture, m) {
    m.doc() = "System audio output capture library";
    py::class_<PyAudioCapture>(m, "AudioCapture")
        .def(py::init<>())
        .def("initialize", &PyAudioCapture::initialize, "Initialize audio capture")
        .def("start_capture", &PyAudioCapture::start_capture, "Start audio capture")
        .def("stop_capture", &PyAudioCapture::stop_capture, "Stop audio capture")
        .def("set_audio_callback", &PyAudioCapture::set_audio_callback, "Set audio data callback")
        .def("get_sample_rate", &PyAudioCapture::get_sample_rate, "Get audio sample rate")
        .def("is_running", &PyAudioCapture::is_running, "Check if capture is running");
    m.attr("__version__") = "1.0.0";
}