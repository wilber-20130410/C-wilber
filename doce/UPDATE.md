
总项目更新版本号： v1.0.1-beta-3-build-0301144501.2026

本项目更新版本号： v1.0.1-build-140200301144501.2026

### 新增

#### audio_capture

- 1.新增audio_capture_dll_export.h，提供AudioCapture类的DLL导出/导入宏、完整公开接口声明
- 2.新增Windows UTF-8编码编译选项（C4819）
- 3.新增Windows多线程编译选项
- 4.新增pybind11路径自动检测（含绝对路径/用户目录备选）（C1089）
- 5.新增平台专属备选音频库链接（Windows: ksuser；Linux: pthread）
- 6.新增Python扩展后缀自动适配逻辑（Windows: .pyd；Linux/macOS: .so）
- 7.新增编译配置信息打印（编译器/Python版本/输出路径）
- 8.新增ALSA捕获线程错误恢复逻辑（欠载/断连）

### 修复

#### audio_capture

- 1.修复PulseAudio `pa_threaded_mainloop_wait_timeout` 函数兼容
- 2.修复MSBuild `set_target_properties` 找不到audio_capture目标
- 3.修复MSBuild 识别`AUDIO_PLATFORM_WINDOWS`为源文件（C1083）
- 4.修复MSBuild 不支持`-fPIC`编译选项（D9002）
- 5.修复宏重定义（`AUDIO_PLATFORM_WINDOWS`）（C4005）
- 6.修复ALSA初始化后未调用`snd_pcm_prepare`
- 7.修复PulseAudio初始化无超时机制 
- 8.修复lambda表达式可见性警告（py_audio_capture.cpp）
- 9.修复可视化中文字体缺失警告
- 10.确保所有代码都符合标准 C++ 规范

### 变更

#### audio_capture

- 1.CMake最低版本提升至3.14
- 2.输出目录统一
- 3.音频数据回调添加全零数据过滤
- 4.CMake目标类型调整（`MODULE`替代`SHARED`）
- 5.PulseAudio上下文状态双重检查
- 6.清理资源逻辑封装（`cleanup()`方法）
- 7.音频格式参数容错（采样率/声道数自动适配）
- 8.移除了 GCC 特有的 `__attribute__((visibility("default")))`
- 9.简化了 lambda 表达式的语法

### 其他

- 无



by:wilber-20130410

2026.3.1