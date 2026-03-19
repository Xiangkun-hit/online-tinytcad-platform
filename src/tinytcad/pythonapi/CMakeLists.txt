# 找到 pybind11
find_package(pybind11 REQUIRED)

# 生成 Python 可调用模块
pybind11_add_module(tinytcad_py python_bind.cpp)

# 头文件路径（关联 src 源码）
target_include_directories(tinytcad_py PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/solver
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/io
)

# 链接核心库
target_link_libraries(tinytcad_py PRIVATE mesh solver utils io)