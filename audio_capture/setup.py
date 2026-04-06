# 什么辣鸡cmake，写了半天编译出来连依赖都处理不好，还是python的setuptools好用，直接写个setup.py就能编译出一个可用的库了，还能自动处理依赖
# 编译命令: python setup.py build_ext --inplace
from setuptools import setup, Extension
import pybind11
import pybind11.setup_helpers

ext_module = Extension(
    'audio_capture',
    sources=['audio_capture.cpp', 'py_audio_capture.cpp'],
    include_dirs=[pybind11.get_include()],
    libraries=['ole32', 'avrt', 'ksuser'],
    extra_compile_args=['/utf-8', '/std:c++14'],
)

setup(
    name='audio_capture',
    version='1.0.1',
    description='Cross-platform audio capture library',
    ext_modules=[ext_module],
    cmdclass={'build_ext': pybind11.setup_helpers.build_ext},
)