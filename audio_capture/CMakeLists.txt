cmake_minimum_required(VERSION 3.12)
project(AudioCapture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED HINTS "${Python3_SITELIB}")

if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found via CONFIG, trying MODULE mode")
    find_package(pybind11 MODULE REQUIRED)
endif()

message(STATUS "Python found: ${Python_EXECUTABLE}")
message(STATUS "Python include dirs: ${Python_INCLUDE_DIRS}")
message(STATUS "Python library: ${Python_LIBRARIES}")

include_directories(${CMAKE_SOURCE_DIR}/include)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0600)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_USE_MATH_DEFINES)
    set(WINDOWS_LIBS ole32 avrt winmm dsound)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_library(audio_capture_shared SHARED
    src/audio_capture.cpp
)

set_target_properties(audio_capture_shared PROPERTIES
    OUTPUT_NAME "audio_capture"
    DEFINE_SYMBOL "AUDIO_CAPTURE_EXPORTS"
)

if(WIN32)
    target_link_libraries(audio_capture_shared PRIVATE ${WINDOWS_LIBS})
endif()

pybind11_add_module(audio_capture 
    src/py_audio_capture.cpp
)

set_target_properties(audio_capture PROPERTIES
    SUFFIX ".pyd"
    OUTPUT_NAME "audio_capture_1"
)

target_link_libraries(audio_capture PRIVATE 
    audio_capture_shared
    pybind11::headers
)

if(WIN32)
    target_link_libraries(audio_capture PRIVATE ${WINDOWS_LIBS})
endif()

install(TARGETS audio_capture_shared
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(TARGETS audio_capture
    LIBRARY DESTINATION python
    RUNTIME DESTINATION python
)

if(WIN32)
    add_custom_command(TARGET audio_capture POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:audio_capture_shared>
        $<TARGET_FILE_DIR:audio_capture>
        COMMENT "Copying audio_capture DLL to Python module directory"
    )
endif()