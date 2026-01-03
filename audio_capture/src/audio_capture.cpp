#include "audio_capture.h"
#include <iostream>
#include <cstring>

AudioCapture::AudioCapture() : isCapturing(false) {}

AudioCapture::~AudioCapture() {
    stopCapture();
}

bool AudioCapture::initialize() {
#ifdef _WIN32
    HRESULT hr;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM" << std::endl;
        return false;
    }
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
    if (FAILED(hr)) {
        std::cerr << "Failed to create device enumerator" << std::endl;
        return false;
    }
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &audioDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to get default audio endpoint" << std::endl;
        return false;
    }
    hr = audioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);
    if (FAILED(hr)) {
        std::cerr << "Failed to activate audio client" << std::endl;
        return false;
    }
    hr = audioClient->GetMixFormat(&waveFormat);
    if (FAILED(hr)) {
        std::cerr << "Failed to get mix format" << std::endl;
        return false;
    }
    sampleRate = waveFormat->nSamplesPerSec;
    channels = waveFormat->nChannels;
    std::cout << "Sample rate: " << sampleRate << ", Channels: " << channels << std::endl;
    return true;
#else
    std::cerr << "Windows is currently the only supported platform." << std::endl;
    return false;
#endif
}

bool AudioCapture::startCapture() {
    if (isCapturing) {
        return true;
    }
#ifdef _WIN32
    HRESULT hr;
    hr = audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0, 0, waveFormat, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize audio client" << std::endl;
        return false;
    }
    hr = audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient);
    if (FAILED(hr)) {
        std::cerr << "Failed to get audio capture client" << std::endl;
        return false;
    }
    hr = audioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "Failed to start audio client" << std::endl;
        return false;
    }
    isCapturing = true;
    captureThreadHandle = std::thread(&AudioCapture::captureThread, this);
    return true;
#else
    return false;
#endif
}

void AudioCapture::stopCapture() {
    if (!isCapturing) {
        return;
    }
    isCapturing = false;
    if (captureThreadHandle.joinable()) {
        captureThreadHandle.join();
    }
#ifdef _WIN32
    if (audioClient) {
        audioClient->Stop();
    }
    if (captureClient) {
        captureClient->Release();
        captureClient = nullptr;
    }
    if (audioClient) {
        audioClient->Release();
        audioClient = nullptr;
    }
    if (audioDevice) {
        audioDevice->Release();
        audioDevice = nullptr;
    }
    if (deviceEnumerator) {
        deviceEnumerator->Release();
        deviceEnumerator = nullptr;
    }
    if (waveFormat) {
        CoTaskMemFree(waveFormat);
        waveFormat = nullptr;
    }
    CoUninitialize();
#endif
}

void AudioCapture::setAudioCallback(std::function<void(const std::vector<float>&)> callback) {
    audioCallback = callback;
}

void AudioCapture::captureThread() {
#ifdef _WIN32
    const UINT32 bufferDuration = 100000; // 100ms
    UINT32 packetLength = 0;
    BYTE* data = nullptr;
    UINT32 numFramesAvailable = 0;
    DWORD flags = 0;
    while (isCapturing) {
        Sleep(10);
        HRESULT hr = captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            continue;
        }
        while (packetLength > 0) {
            hr = captureClient->GetBuffer(&data, &numFramesAvailable, &flags, nullptr, nullptr);
            if (SUCCEEDED(hr) && data != nullptr) {
                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && audioCallback) {
                    std::vector<float> audioData;
                    size_t sampleCount = numFramesAvailable * channels;
                    if (waveFormat->wBitsPerSample == 16) {
                        int16_t* samples = reinterpret_cast<int16_t*>(data);
                        audioData.resize(sampleCount);
                        for (size_t i = 0; i < sampleCount; ++i) {
                            audioData.push_back(samples[i] / 32768.0f);
                        }
                    } else if (waveFormat->wBitsPerSample == 32) {
                        float* samples = reinterpret_cast<float*>(data);
                        audioData.assign(samples, samples + sampleCount);
                    }
                    if (!audioData.empty()) {
                        audioCallback(audioData);
                    }
                }
                captureClient->ReleaseBuffer(numFramesAvailable);
            }
            hr = captureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) {
                break;
            }
        }
    }
#endif
}    