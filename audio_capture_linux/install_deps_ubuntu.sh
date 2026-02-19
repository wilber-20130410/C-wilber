#!/bin/bash
echo "安装Ubuntu音频捕获库依赖..."

# 更新包列表
sudo apt-get update

# 安装开发工具
sudo apt-get install -y build-essential cmake pkg-config

# 安装Python开发包
sudo apt-get install -y python3-dev python3-pip

# 安装音频库
sudo apt-get install -y libpulse-dev libasound2-dev

# 安装Python包
pip3 install pybind11 numpy matplotlib pygame

# 可选：安装Jack音频服务器
# sudo apt-get install -y jackd2 libjack-jackd2-dev

echo "依赖安装完成！"