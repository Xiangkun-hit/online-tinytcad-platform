#!/bin/bash
# tinytcad 一键编译&运行脚本

# 1. 如果存在build文件夹，先清理
if [ -d "build" ]; then
    echo "清理旧编译文件..."
    rm -rf build
fi

# 2. 创建新的build目录
mkdir build && cd build

# 3. CMake配置（默认Release模式，高性能）
cmake ..

# 4. 多线程编译
make -j4

# 5. 运行程序
echo "================ 运行程序 ================"
./src/tinytcad