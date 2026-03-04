#!/bin/bash
# build.sh - 编译脚本（修复：绝对路径+sudo清理）

# 关键：用普通用户的绝对路径，避免sudo时~解析为/root
PROJECT_DIR="/home/HwHiAiUser/2026winter/Orangepi_Core_Project"
cd ${PROJECT_DIR} || { echo "错误：进入项目目录失败！目录不存在：${PROJECT_DIR}"; exit 1; }

# 清理build目录（加sudo，解决权限问题，确保彻底清理）
sudo rm -rf build
mkdir build
cd build || { echo "错误：进入build目录失败！"; exit 1; }

# 配置CMake（基于正确的项目根目录..）
cmake .. \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 编译（-j$(nproc)利用所有核心，加速编译）
make -j$(nproc)

# 如果编译成功，运行程序
if [ -f Orangepi_Core_Project ]; then
    echo -e "\033[32m编译成功！运行程序...\033[0m"
    ./Orangepi_Core_Project 1  # 参数1：检测模式
else
    echo -e "\033[31m编译失败！\033[0m"
    exit 1  # 新增：编译失败返回错误码，让自动化脚本识别
fi