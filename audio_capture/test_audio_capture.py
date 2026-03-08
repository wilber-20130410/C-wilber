#!/usr/bin/env python3

'''
名称:test_audio_capture.py
作者:wilber-20130410
版权: © 2025~2026 wilber-20130410
版本:1.0.1[140200301144501](正式版)
日期:2026.2.28
留言:
1.本代码仅供学习交流使用,请勿用于商业用途。
2.本代码参考了网络上部分代码,在此表示感谢。
3.本人推荐使用Visual Studio Code作为IDE(集成开发环境)。
4.如果您有建议、发现了Bug、问题或者您进行优化后的代码,欢迎向本人邮箱xuwb0410@163.com发送邮件,本人将在21天内进行回复。
5.本代码适用于所有操作系统。
6.以上留言不分先后。
'''

# 跨平台音频捕获库测试

import audio_capture
import numpy as np
import time
import sys

def print_separator():
    print("=" * 60)

def test_audio_capture():
    print_separator()
    print(f"测试跨平台音频捕获库")
    print(f"Python版本: {sys.version}")
    print(f"平台: {audio_capture.__platform__}")
    print(f"库版本: {audio_capture.__version__}")
    print_separator()
    
    # 创建捕获对象
    capture = audio_capture.AudioCapture()
    
    # 测试可用设备
    print("\n可用输入设备:")
    devices = capture.get_input_devices()
    for i, device in enumerate(devices):
        print(f"  {i}: {device}")
    print_separator()
    
    # 测试初始化
    print("\n初始化音频捕获...")
    if capture.initialize(audio_capture.Backend.AUTO):
        print(f"  ✓ 初始化成功")
        print(f"    后端: {capture.backend_name}")
        print(f"    格式: {capture.format}")
    else:
        print("  ✗ 初始化失败")
        return
    
    print_separator()
    
    # 测试音频回调
    received_data = []
    start_time = time.time()
    
    def audio_callback(data, format):
        """音频数据回调函数"""
        nonlocal start_time
        received_data.extend(data[:100])  # 只保存部分数据
        
        # 限制数据长度
        if len(received_data) > 5000:
            received_data[:1000] = []
        
        # 每秒显示统计信息
        current_time = time.time()
        if current_time - start_time >= 1.0:
            if received_data:
                arr = np.array(received_data[-1000:])
                print(f"    样本数: {len(arr)}")
                print(f"    最大值: {np.max(np.abs(arr)):.4f}")
                print(f"    均值: {np.mean(arr):.6f}")
                print(f"    标准差: {np.std(arr):.6f}")
            start_time = current_time
    
    # 设置回调
    capture.set_audio_callback(audio_callback)
    
    # 开始捕获
    print("\n开始音频捕获 (5秒)...")
    if capture.start_capture():
        print("  ✓ 捕获开始")
        
        # 捕获5秒钟
        for i in range(5):
            print(f"  第 {i+1} 秒")
            time.sleep(1)
        
        # 停止捕获
        capture.stop_capture()
        print("  ✓ 捕获停止")
    else:
        print("  ✗ 无法开始捕获")
    
    print_separator()
    
    # 显示统计信息
    if received_data:
        data_array = np.array(received_data)
        print(f"\n捕获统计:")
        print(f"  总样本数: {len(data_array)}")
        print(f"  最大值: {np.max(data_array):.4f}")
        print(f"  最小值: {np.min(data_array):.4f}")
        print(f"  均值: {np.mean(data_array):.6f}")
        print(f"  标准差: {np.std(data_array):.6f}")
    else:
        print("\n⚠ 未收到音频数据")
    
    print_separator()
    print("测试完成")
    print_separator()

def visualize_audio():
    """简单的实时可视化"""
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
    
    print("\n启动实时可视化...")
    
    capture = audio_capture.AudioCapture()
    
    if not capture.initialize():
        print("初始化失败")
        return
    
    # 创建图表
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle(f"实时音频可视化 - {capture.backend_name}")
    
    x_data = np.arange(1000)
    line1, = ax1.plot(x_data, np.zeros(1000), 'b-')
    ax1.set_title("波形")
    ax1.set_xlim(0, 1000)
    ax1.set_ylim(-1, 1)
    ax1.grid(True)
    
    line2, = ax2.plot(x_data[:512], np.zeros(512), 'r-')
    ax2.set_title("频谱")
    ax2.set_xlim(0, 512)
    ax2.set_ylim(0, 0.1)
    ax2.grid(True)
    
    audio_buffer = []
    
    def callback(data, format):
        nonlocal audio_buffer
        audio_buffer.extend(data)
        if len(audio_buffer) > 2000:
            audio_buffer = audio_buffer[-2000:]
    
    def update(frame):
        if audio_buffer:
            # 更新波形
            waveform = audio_buffer[-1000:]
            if len(waveform) < 1000:
                waveform = np.pad(waveform, (0, 1000 - len(waveform)), 'constant')
            line1.set_ydata(waveform)
            
            # 计算频谱
            if len(audio_buffer) >= 1024:
                data = np.array(audio_buffer[-1024:])
                window = np.hanning(len(data))
                spectrum = np.abs(np.fft.fft(data * window))
                spectrum = spectrum[:512]
                line2.set_ydata(spectrum)
        
        return line1, line2
    
    capture.set_audio_callback(callback)
    capture.start_capture()
    
    ani = FuncAnimation(fig, update, interval=50, blit=True)
    plt.tight_layout()
    plt.show()
    
    capture.stop_capture()

if __name__ == "__main__":
    # 运行测试
    test_audio_capture()
    
    # 询问是否进行可视化
    response = input("\n是否启动实时可视化? (y/n): ")
    if response.lower() == 'y':
        visualize_audio()