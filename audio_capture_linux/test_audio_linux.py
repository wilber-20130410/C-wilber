import audio_capture
import numpy as np
import time

print("测试修复的音频捕获库...")

# 创建捕获对象
capture = audio_capture.AudioCapture()

# 初始化
if capture.initialize(audio_capture.Backend.AUTO):
    print(f"初始化成功")
    print(f"后端: {capture.get_backend_name()}")
    print(f"采样率: {capture.get_sample_rate()} Hz")
    print(f"声道数: {capture.get_channels()}")
    
    # 存储接收到的数据
    received_data = []
    
    def audio_callback(data):
        # 这个函数在C++线程中被调用
        received_data.extend(data[:100])  # 只保存部分数据
        if len(received_data) > 1000:
            received_data.pop(0)
    
    # 设置回调
    capture.set_audio_callback(audio_callback)
    
    # 开始捕获
    if capture.start_capture():
        print("开始捕获音频...")
        
        try:
            # 捕获5秒钟
            for i in range(5):
                print(f"  第 {i+1} 秒")
                time.sleep(1)
                
                # 显示一些统计信息
                if received_data:
                    arr = np.array(received_data)
                    print(f"    收到 {len(arr)} 个样本")
                    print(f"    范围: [{arr.min():.4f}, {arr.max():.4f}]")
                    print(f"    均值: {arr.mean():.6f}")
                else:
                    print("    尚未收到数据")
        finally:
            # 确保停止捕获
            capture.stop_capture()
            print("捕获已停止")
            
            if received_data:
                print(f"总共收到 {len(received_data)} 个样本")
            else:
                print("未收到任何数据")
    else:
        print("无法开始捕获")
else:
    print("初始化失败")

print("测试完成")