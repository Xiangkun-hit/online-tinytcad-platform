# tinytcad - 微型 TCAD 半导体仿真框架
![C++](https://img.shields.io/badge/C++-17/20-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

### 项目简介
基于现代 C++ 开发的一维数值仿真框架，集成网格生成、数值求解、结果输出与 Python 调用接口，支持线性模型、二次模型、泊松（POSSION）方程三种仿真场景，适用于教学、科研与工程验证。

### 核心功能
✅ **模块化架构**：Utils（工具）/ Mesh1D（网格）/ Solver（求解器）/ IO（文件）全模块解耦  
✅ **自定义网格**：支持命令行配置节点数、坐标起始/结束范围  
✅ **数值求解引擎**：对接网格数据，模型自由切换，支持扩展真实半导体物理模型（泊松方程/载流子输运）  
✅ **命令行交互**：无需修改代码，终端直接传参，工业级工具标准  
✅ **结果导出**：生成标准化 `result.txt`，支持仿真数据可视化对接  
✅ **工程化规范**：CMake 构建、Git 版本管理、.gitignore 隔离、统一代码注释  
✅ **分级日志系统**：彩色输出 + 时间戳，支持INFO/SUCCESS/ERROR三级日志  
✅ **现代C++优化**：禁用拷贝语义 + 移动语义，杜绝内存拷贝，高性能  
✅ **一键编译运行脚本** : Python 一键调用 C++ 核心算法


### 技术栈
- 编程语言：C++17（STL 容器、面向对象、RAII 机制）
- 构建工具：CMake 3.20+
- 版本控制：Git/GitHub
- 核心设计：面向对象、RAII、模块化、禁用拷贝/移动语义

### 快速开始
#### 环境依赖
- Linux (Ubuntu 20.04+/CentOS) 
- CMake 3.20 及以上
- C++ 编译器 (GCC 9+/Clang 10+/MSVC 19.28+)

#### 编译运行（Ubuntu 示例）

```bash
# 1. 克隆仓库
git clone git@github.com:Xiangkun-hit/tinytcad.git
cd tinytcad

### 一键编译运行（推荐）

chmod +x build.sh
./build.sh

### 手动编译运行
# 2. 创建构建目录
mkdir build && cd build

# 3. CMake 配置
cmake ..

# 4. 编译（-j4 对应CPU核心数，加速编译）
make -j4

# 5. 运行仿真程序
src/tinytcad
```

### 项目目录结构
tinytcad/
├── src/                # 主源码目录  
│   ├── main.cpp        # 程序入口 + 命令行解析  
│   ├── utils/          # 日志与工具模块  
│   ├── mesh/           # 网格生成模块  
│   ├── solver/         # 求解器模块  
│   └── io/             # 文件输出模块  
├── build.sh            # 一键编译运行脚本  
├── CMakeLists.txt      # 项目构建配置  
└── README.md           # 项目说明  
