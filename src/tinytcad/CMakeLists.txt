cmake_minimum_required(VERSION 3.16)

# 项目名称：tinytcad
project(tinytcad)

# 使用 C++17 标准（强制，稳定）
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ========== 设置默认编译模式 ==========
# 如果用户没指定编译类型，默认用 Release（高性能模式）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# ========== 区分Debug / Release 编译优化 ==========
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug 模式：不优化代码 + 生成调试信息（方便找bug）
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
else()
    # Release 模式：开启 O2 级优化（让程序跑更快）
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

# ========== 开启全编译警告 ==========
# -Wall：所有基础警告  -Wextra：额外严格警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 开启测试功能
ENABLE_TESTING()

# 加载三个子模块
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(pythonapi)


