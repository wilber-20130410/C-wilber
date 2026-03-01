#!/bin/bash

# 名称:build.sh
# 作者:wilber-20130410
# 版权: © 2025~2026 wilber-20130410
# 版本:1.0.1[140200301144501](正式版)
# 日期:2026.2.28
# 留言:
# 1.本代码仅供学习交流使用,请勿用于商业用途。
# 2.本代码参考了网络上部分代码,在此表示感谢。
# 3.本人推荐使用Visual Studio Code作为IDE(集成开发环境)。
# 4.如果您有建议、发现了Bug、问题或者您进行优化后的代码,欢迎向本人邮箱xuwb0410@163.com发送邮件,本人将在21天内进行回复。
# 5.本代码适用于所有操作系统。
# 6.以上留言不分先后。

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