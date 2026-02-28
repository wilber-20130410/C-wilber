#!/bin/bash

echo "开始编译音频捕获库..."

# 清理旧的构建目录
rm -rf build
mkdir build
cd build

# 运行CMake
cmake ..

# 编译
make -j4

if [ $? -eq 0 ]; then
    echo "编译成功！"
    echo "库文件: $(pwd)/audio_capture.so"
    
    # 测试导入
    cd ..
    python3 -c "import sys; sys.path.insert(0, 'build'); import audio_capture; print('✓ 导入成功')"
else
    echo "编译失败！"
    exit 1
fi