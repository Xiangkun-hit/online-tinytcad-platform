#ifndef TINYTCAD_MESH_H
#define TINYTCAD_MESH_H

#include <vector>

class Mesh1D {
  // 私有成员变量：内部数据，外部不能直接访问（封装特性）
private:
  int node_num_;                 // 网格总节点数
  double x_start_;               // 网格起始坐标
  double x_end_;                 // 网格结束坐标
  std::vector<double> x_coords; // 动态数组：存储所有节点的坐标
  //私有方法：生成网格数据
  void generateMesh();
  //参数校验
  bool check_valid() const;

  // 公有函数：外部可以调用的接口
public:
  // 默认构造：沿用之前的 10 节点 0~1 范围（兼容老代码）
  Mesh1D();
  // 自定义构造：指定节点数、起始坐标、结束坐标
  Mesh1D(int node_num, double start_x, double end_x);

  //禁用拷贝
  Mesh1D(const Mesh1D&) = delete;
  Mesh1D& operator=(const Mesh1D&) = delete;

  //增加 移动构造+移动赋值
  Mesh1D(Mesh1D&&) = default;
  Mesh1D& operator=(Mesh1D&&) = default;
  ~Mesh1D() = default;

  // 打印网格信息
  void print_mesh() const;
  // getter 接口：外部获取网格数据（后面求解器要用）
  int get_node_num() const;
  // 获取数组指针（兼容后续计算）
  std::vector<double>& get_x_coords();
  const std::vector<double>& get_x_coords() const;

  //获取参数（给IO模块使用）
  double get_start_x() const;
  double get_end_x() const;
};

#endif