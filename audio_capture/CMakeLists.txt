cmake_minimum_required(VERSION 3.14)
project(audio_capture)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 核心配置：跨平台库格式支持 ==========
# 默认为共享库（SHARED），自动适配平台后缀（DLL/PYD/SO）
# Windows: 编译为 PYD（Python 扩展）+ 可选 DLL（独立库）
# Linux/macOS: 编译为 SO（Python 扩展 + 共享库）
option(BUILD_PYTHON_EXTENSION "是否编译 Python 扩展（PYD/SO）" ON)
option(BUILD_STANDALONE_DLL "Windows 平台是否额外编译独立 DLL（非必需）" OFF)

# 设置输出目录（统一输出到 build/lib 下，便于查找）
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/static)

# ========== Python 相关配置 ==========
if(BUILD_PYTHON_EXTENSION)
    find_package(Python REQUIRED COMPONENTS Interpreter Development)
    
    # 手动查找 pybind11 路径（兼容不同安装位置）
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import pybind11; print(pybind11.get_include())"
        OUTPUT_VARIABLE PYBIND11_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT PYBIND11_INCLUDE_DIR)
        execute_process(
            COMMAND ${Python_EXECUTABLE} -c "import sys; import os; sys.path.insert(0, os.path.expanduser('~/.local/lib/python*/site-packages')); import pybind11; print(pybind11.get_include())"
            OUTPUT_VARIABLE PYBIND11_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
    if(NOT PYBIND11_INCLUDE_DIR)
        message(FATAL_ERROR "无法找到 pybind11，请安装：pip3 install pybind11")
    endif()
    message(STATUS "找到 pybind11 包含目录: ${PYBIND11_INCLUDE_DIR}")
    
    # 创建 pybind11 接口目标
    add_library(pybind11_interface INTERFACE)
    target_include_directories(pybind11_interface INTERFACE ${PYBIND11_INCLUDE_DIR})
    target_compile_definitions(pybind11_interface INTERFACE 
        PYBIND11_NO_ASSERT_GIL_HELD_INCREF_DECREF
    )
endif()

# ========== 平台特定配置 ==========
set(PLATFORM_LIBS "")
set(PLATFORM_DEFINES "")

if(WIN32)
    # Windows 平台：支持 MSVC/GCC 编译
    add_definitions(-D_WIN32_WINNT=0x0600 -DNOMINMAX -DAUDIO_PLATFORM_WINDOWS)
    set(PLATFORM_LIBS ole32 avrt)  # Windows 音频相关系统库
    
    # 编译选项：启用多线程编译、禁用不安全函数警告
    if(MSVC)
        add_compile_options(/MP /wd4996)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)  # 自动导出 DLL 符号
    else()
        add_compile_options(-Wno-deprecated-declarations)
    endif()

elseif(UNIX AND NOT APPLE)
    # Linux 平台：保持原 PulseAudio/ALSA 配置
    add_definitions(-DAUDIO_PLATFORM_LINUX)
    find_package(PkgConfig REQUIRED)
    
    # 查找 PulseAudio
    pkg_check_modules(PULSEAUDIO libpulse libpulse-simple)
    if(PULSEAUDIO_FOUND)
        message(STATUS "找到 PulseAudio")
        include_directories(${PULSEAUDIO_INCLUDE_DIRS})
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${PULSEAUDIO_LIBRARIES})
        add_definitions(-DHAVE_PULSEAUDIO)
    endif()
    
    # 查找 ALSA
    find_library(ALSA_LIB asound)
    if(ALSA_LIB)
        message(STATUS "找到 ALSA: ${ALSA_LIB}")
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ALSA_LIB})
        add_definitions(-DHAVE_ALSA)
    endif()
    
    # 添加线程库
    set(PLATFORM_LIBS ${PLATFORM_LIBS} pthread)

elseif(APPLE)
    # macOS 平台：保持原 CoreAudio 配置
    add_definitions(-DAUDIO_PLATFORM_MACOS)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIO_TOOLBOX_LIB AudioToolbox)
    set(PLATFORM_LIBS ${COREAUDIO_LIB} ${AUDIO_TOOLBOX_LIB})
    add_definitions(-DHAVE_COREAUDIO)
endif()

# ========== 源文件与包含目录 ==========
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CORE_SOURCES audio_capture.cpp)  # 核心音频捕获逻辑
set(PYTHON_SOURCES py_audio_capture.cpp)  # Python 绑定逻辑

# ========== 目标编译：分类型构建 ==========
if(BUILD_PYTHON_EXTENSION)
    # 1. 编译 Python 扩展（PYD/SO）：Python 直接导入使用
    add_library(audio_capture_python MODULE 
        ${CORE_SOURCES}
        ${PYTHON_SOURCES}
    )
    
    # 设置 Python 扩展属性
    set_target_properties(audio_capture_python PROPERTIES
        PREFIX "${PYTHON_MODULE_PREFIX}"  # Windows 为空，Linux/macOS 为 "lib"
        SUFFIX "${PYTHON_MODULE_EXTENSION}"  # 自动适配：Windows 为 .pyd，Linux 为 .so，macOS 为 .so
        OUTPUT_NAME "audio_capture"  # 最终文件名：audio_capture.pyd/.so
    )
    
    # 链接依赖
    target_link_libraries(audio_capture_python PRIVATE
        pybind11_interface
        ${Python_LIBRARIES}
        ${PLATFORM_LIBS}
    )
    
    # PulseAudio 额外链接选项
    if(PULSEAUDIO_FOUND)
        target_link_libraries(audio_capture_python PRIVATE ${PULSEAUDIO_LDFLAGS})
    endif()
    
    # 编译选项：fPIC 兼容所有平台
    target_compile_options(audio_capture_python PRIVATE -fPIC)
    
    message(STATUS "将编译 Python 扩展：${OUTPUT_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}audio_capture${PYTHON_MODULE_EXTENSION}")
endif()

if(BUILD_STANDALONE_DLL AND WIN32)
    # 2. Windows 额外编译独立 DLL（供 C/C++ 程序直接调用，非 Python 扩展）
    add_library(audio_capture_dll SHARED
        ${CORE_SOURCES}
        # 新增 DLL 导出头文件（需手动创建，示例见下文）
        audio_capture_dll_export.h
    )
    
    # 设置 DLL 属性
    set_target_properties(audio_capture_dll PROPERTIES
        OUTPUT_NAME "audio_capture"
        SUFFIX ".dll"
        WINDOWS_EXPORT_ALL_SYMBOLS OFF  # 禁用自动导出，使用显式导出宏
    )
    
    # 链接依赖
    target_link_libraries(audio_capture_dll PRIVATE
        ${PLATFORM_LIBS}
    )
    
    # 编译选项：定义导出宏
    target_compile_definitions(audio_capture_dll PRIVATE
        AUDIO_CAPTURE_DLL_EXPORT  # 用于头文件中控制导出/导入
    )
    
    message(STATUS "将编译独立 DLL：${OUTPUT_DIR}/audio_capture.dll")
endif()

# ========== 辅助配置 ==========
# 复制测试文件到输出目录
if(BUILD_PYTHON_EXTENSION)
    configure_file(test_audio_capture.py ${OUTPUT_DIR}/test_audio_capture.py COPYONLY)
endif()

# 显示最终配置信息
message(STATUS "=")
message(STATUS "配置完成:")
message(STATUS "  操作系统: ${CMAKE_SYSTEM_NAME}")
if(BUILD_PYTHON_EXTENSION)
    message(STATUS "  Python 版本: ${Python_VERSION}")
    message(STATUS "  pybind11 路径: ${PYBIND11_INCLUDE_DIR}")
endif()
message(STATUS "  平台库: ${PLATFORM_LIBS}")
message(STATUS "  输出目录: ${OUTPUT_DIR}")
message(STATUS "  编译目标:")
if(BUILD_PYTHON_EXTENSION)
    message(STATUS "    - Python 扩展: audio_capture${PYTHON_MODULE_EXTENSION}")
endif()
if(BUILD_STANDALONE_DLL AND WIN32)
    message(STATUS "    - 独立 DLL: audio_capture.dll")
endif()
message(STATUS "=")