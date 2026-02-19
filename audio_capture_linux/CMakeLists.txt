cmake_minimum_required(VERSION 3.12)
project(AudioCaptureLinux)

# 修改为C++14标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 查找Python和pybind11
list(APPEND CMAKE_PREFIX_PATH "/usr/lib/python3/dist-packages/pybind11/share/cmake/pybind11")
find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

# 查找音频库
find_library(PULSE_LIB pulse)
find_library(PULSE_SIMPLE_LIB pulse-simple)
find_library(ALSA_LIB asound)

if(PULSE_LIB AND PULSE_SIMPLE_LIB)
    set(HAVE_PULSEAUDIO TRUE)
    set(LIBS ${LIBS} ${PULSE_LIB} ${PULSE_SIMPLE_LIB})
    message(STATUS "找到PulseAudio库")
else()
    message(WARNING "未找到PulseAudio库")
endif()

if(ALSA_LIB)
    set(HAVE_ALSA TRUE)
    set(LIBS ${LIBS} ${ALSA_LIB})
    message(STATUS "找到ALSA库")
else()
    message(WARNING "未找到ALSA库")
endif()

# 包含目录
include_directories(${Python_INCLUDE_DIRS})
include_directories(${pybind11_INCLUDE_DIRS})

# 主音频捕获库
add_library(audio_capture_linux SHARED
    audio_capture_linux.cpp
)

if(HAVE_PULSEAUDIO)
    target_compile_definitions(audio_capture_linux PRIVATE HAVE_PULSEAUDIO)
endif()

if(HAVE_ALSA)
    target_compile_definitions(audio_capture_linux PRIVATE HAVE_ALSA)
endif()

target_link_libraries(audio_capture_linux PRIVATE ${LIBS})

# Python模块
pybind11_add_module(audio_capture
    py_audio_capture_linux.cpp
)

target_link_libraries(audio_capture PRIVATE 
    audio_capture_linux
    pybind11::headers
    ${LIBS}
)

# 设置目标属性
set_target_properties(audio_capture PROPERTIES
    OUTPUT_NAME "audio_capture"
    SUFFIX ".so"
)

# 复制测试文件
configure_file(test_audio_linux.py ${CMAKE_BINARY_DIR}/test_audio_linux.py COPYONLY)