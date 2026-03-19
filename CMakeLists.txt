cmake_minimum_required(VERSION 3.10)
project(online_tinytcad_platform)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件路径
include_directories(src/webserver)
include_directories(src/tinytcad)
include_directories(src/tinytcad/src/mesh)
include_directories(src/tinytcad/src/solver)

# 自动搜集所有源码
file(GLOB WEB_SRC src/webserver/*.cpp src/webserver/*.h)
file(GLOB TINYTCAD_SRC src/tinytcad/*.cpp src/tinytcad/*.h)

# 生成最终可执行文件
add_executable(online_platform ${WEB_SRC} ${TINYTCAD_SRC})

# 线程库（WebServer需要）
find_package(Threads REQUIRED)
target_link_libraries(online_platform PRIVATE Threads::Threads)