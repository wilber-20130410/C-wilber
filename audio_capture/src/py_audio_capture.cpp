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