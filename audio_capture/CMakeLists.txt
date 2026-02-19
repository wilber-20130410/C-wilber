cmake_minimum_required(VERSION 3.14)
project(audio_capture)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 查找Python
find_package(Python REQUIRED COMPONENTS Interpreter Development)

# 手动设置pybind11路径 - 不使用find_package
# 直接使用Python的site-packages路径
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import pybind11; print(pybind11.get_include())"
    OUTPUT_VARIABLE PYBIND11_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT PYBIND11_INCLUDE_DIR)
    # 如果上面的命令失败，尝试备选方法
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import sys; import os; sys.path.insert(0, os.path.expanduser('~/.local/lib/python*/site-packages')); import pybind11; print(pybind11.get_include())"
        OUTPUT_VARIABLE PYBIND11_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(NOT PYBIND11_INCLUDE_DIR)
    message(FATAL_ERROR "无法找到pybind11。请安装: pip3 install pybind11")
endif()

message(STATUS "找到pybind11包含目录: ${PYBIND11_INCLUDE_DIR}")

# 创建pybind11接口目标
add_library(pybind11_interface INTERFACE)
target_include_directories(pybind11_interface INTERFACE ${PYBIND11_INCLUDE_DIR})
target_compile_definitions(pybind11_interface INTERFACE 
    PYBIND11_NO_ASSERT_GIL_HELD_INCREF_DECREF
)

# 平台特定设置
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0600 -DNOMINMAX)
    set(PLATFORM_LIBS ole32 avrt)
elseif(UNIX AND NOT APPLE)
    # Linux - 查找PulseAudio和ALSA
    find_package(PkgConfig REQUIRED)
    
    set(PLATFORM_LIBS "")
    
    pkg_check_modules(PULSEAUDIO libpulse libpulse-simple)
    if(PULSEAUDIO_FOUND)
        message(STATUS "找到PulseAudio")
        include_directories(${PULSEAUDIO_INCLUDE_DIRS})
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${PULSEAUDIO_LIBRARIES})
        add_definitions(-DHAVE_PULSEAUDIO)
    endif()
    
    find_library(ALSA_LIB asound)
    if(ALSA_LIB)
        message(STATUS "找到ALSA: ${ALSA_LIB}")
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ALSA_LIB})
        add_definitions(-DHAVE_ALSA)
    endif()
    
    # 添加pthread
    set(PLATFORM_LIBS ${PLATFORM_LIBS} pthread)
    
elseif(APPLE)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIO_TOOLBOX_LIB AudioToolbox)
    set(PLATFORM_LIBS ${COREAUDIO_LIB} ${AUDIO_TOOLBOX_LIB})
    add_definitions(-DHAVE_COREAUDIO)
endif()

# 包含目录
include_directories(${Python_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 源文件
set(SOURCES
    audio_capture.cpp
    py_audio_capture.cpp
)

# 创建Python模块
add_library(audio_capture MODULE ${SOURCES})

# 设置目标属性
set_target_properties(audio_capture PROPERTIES
    PREFIX "${PYTHON_MODULE_PREFIX}"
    SUFFIX ".so"
    OUTPUT_NAME "audio_capture"
)

# 链接库
target_link_libraries(audio_capture PRIVATE
    pybind11_interface
    ${Python_LIBRARIES}
    ${PLATFORM_LIBS}
)

# 如果PulseAudio有额外的链接库
if(PULSEAUDIO_FOUND)
    target_link_libraries(audio_capture PRIVATE ${PULSEAUDIO_LDFLAGS})
endif()

# 设置编译选项
target_compile_options(audio_capture PRIVATE -fPIC)

# 复制测试文件
configure_file(test_audio_capture.py ${CMAKE_BINARY_DIR}/test_audio_capture.py COPYONLY)

# 显示配置信息
message(STATUS "配置完成:")
message(STATUS "  Python版本: ${Python_VERSION}")
message(STATUS "  pybind11路径: ${PYBIND11_INCLUDE_DIR}")
message(STATUS "  平台库: ${PLATFORM_LIBS}")