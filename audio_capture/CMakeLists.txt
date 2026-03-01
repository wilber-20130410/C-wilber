cmake_minimum_required(VERSION 3.14)
project(audio_capture)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========== 基础配置 ==========
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ========== Python & pybind11 配置（简化版）==========
find_package(Python REQUIRED COMPONENTS Interpreter Development)

# 使用 pybind11 的官方查找模块
find_package(pybind11 CONFIG REQUIRED)

# 如果上面的方法找不到，使用 Python 模块方式
if(NOT pybind11_FOUND)
    message(STATUS "尝试通过 Python 模块查找 pybind11...")
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import pybind11; print(pybind11.get_include())"
        OUTPUT_VARIABLE PYBIND11_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(PYBIND11_INCLUDE_DIR)
        message(STATUS "找到 pybind11 包含路径: ${PYBIND11_INCLUDE_DIR}")
        add_library(pybind11::headers INTERFACE IMPORTED)
        set_target_properties(pybind11::headers PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PYBIND11_INCLUDE_DIR}"
        )
    else()
        message(FATAL_ERROR 
            "pybind11 未找到！请安装: pip install pybind11\n"
            "或者手动设置 PYBIND11_INCLUDE_DIR 路径")
    endif()
endif()

# ========== 平台特定配置 ==========
set(PLATFORM_LIBS "")
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0600 -DNOMINMAX -DAUDIO_PLATFORM_WINDOWS)
    set(PLATFORM_LIBS ole32 avrt ksuser)
    add_compile_options(/MP /utf-8 /wd4819 /wd4996)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSEAUDIO libpulse libpulse-simple)
    if(PULSEAUDIO_FOUND)
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${PULSEAUDIO_LIBRARIES})
        add_definitions(-DHAVE_PULSEAUDIO)
    endif()
    find_library(ALSA_LIB asound)
    if(ALSA_LIB)
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ALSA_LIB})
        add_definitions(-DHAVE_ALSA)
    endif()
    set(PLATFORM_LIBS ${PLATFORM_LIBS} pthread)
elseif(APPLE)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIO_TOOLBOX_LIB AudioToolbox)
    set(PLATFORM_LIBS ${COREAUDIO_LIB} ${AUDIO_TOOLBOX_LIB})
    add_definitions(-DHAVE_COREAUDIO)
endif()

# ========== 源文件 ==========
set(SOURCES
    audio_capture.cpp
    py_audio_capture.cpp
)

# ========== 创建目标 ==========
add_library(audio_capture MODULE ${SOURCES})

# ========== 目标属性配置 ==========
if(WIN32)
    set_target_properties(audio_capture PROPERTIES
        PREFIX ""
        SUFFIX ".pyd"
        OUTPUT_NAME "audio_capture"
    )
else()
    set_target_properties(audio_capture PROPERTIES
        PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}"
        OUTPUT_NAME "audio_capture"
    )
    target_compile_options(audio_capture PRIVATE -fPIC)
endif()

# ========== 链接依赖库 ==========
target_link_libraries(audio_capture PRIVATE
    ${Python_LIBRARIES}
    ${PLATFORM_LIBS}
)

# 链接 pybind11
if(TARGET pybind11::headers)
    target_link_libraries(audio_capture PRIVATE pybind11::headers)
elseif(TARGET pybind11::module)
    target_link_libraries(audio_capture PRIVATE pybind11::module)
else()
    # 手动添加包含目录
    target_include_directories(audio_capture PRIVATE ${PYBIND11_INCLUDE_DIR})
endif()

# 添加 Python 包含目录
target_include_directories(audio_capture PRIVATE ${Python_INCLUDE_DIRS})

# PulseAudio 额外链接
if(PULSEAUDIO_FOUND)
    target_include_directories(audio_capture PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    target_link_options(audio_capture PRIVATE ${PULSEAUDIO_LDFLAGS})
endif()

# ========== 复制测试文件 ==========
configure_file(test_audio_capture.py ${CMAKE_BINARY_DIR}/test_audio_capture.py COPYONLY)

# ========== 打印配置信息 ==========
message(STATUS "==================== 配置完成 ====================")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Python版本: ${Python_VERSION}")
message(STATUS "Python包含路径: ${Python_INCLUDE_DIRS}")
message(STATUS "pybind11路径: ${PYBIND11_INCLUDE_DIR}")
message(STATUS "平台库: ${PLATFORM_LIBS}")
message(STATUS "输出文件: ${CMAKE_BINARY_DIR}/audio_capture${CMAKE_SHARED_LIBRARY_SUFFIX}")