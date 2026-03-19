#include "mesh.h"
#include <iostream>
#include "../utils/utils.h"

// 构造函数：初始化默认参数
Mesh1D::Mesh1D() : node_num_(10), x_start_(0.0), x_end_(1.0){
  // vector 自动初始化，无需手动置空
  if(!check_valid()){
    // std::cerr << "错误：默认网格参数非法！" << std::endl;
    //日志+退出
    Utils utils;
    utils.log_error("错误：默认网格参数非法！");
    exit(1);
  }
  generateMesh();
}

// 自定义构造：指定节点数、起始坐标、结束坐标
Mesh1D::Mesh1D(int node_num, double start_x, double end_x)
  : node_num_(node_num),x_start_(start_x),x_end_(end_x){
  if(!check_valid()){
  //std::cerr << "错误:网格参数非法!节点数≥2,起始坐标<结束坐标" << std::endl;
  //日志+退出
  Utils utils;
  utils.log_error("错误:网格参数非法!节点数≥2,起始坐标<结束坐标");
  exit(1);
  }
  generateMesh();
}

//参数合法性校验（核心）
bool Mesh1D::check_valid() const{
  if(node_num_ < 2) return false;
  if(x_start_ >= x_end_) return false;
  return true;  
}

// ======================
// 核心：生成均匀网格（vector版本）
void Mesh1D::generateMesh() {
  // ✅ 重置vector大小
  x_coords.resize(node_num_);
  double step = (x_end_ - x_start_) / (node_num_ - 1);

  for (int i = 0; i < node_num_; ++i) {
    x_coords[i] = x_start_ + i * step;
  }
}

// 打印网格信息
void Mesh1D::print_mesh() const{
  std::cout << "[Mesh1D]网格节点数量:" << node_num_ << std::endl;
  std::cout << "[Mesh1D]坐标范围:[" << x_start_ << "," << x_end_ << "]"
            << std::endl;
  std::cout << "[Mesh1D]网格节点坐标:" << std::endl;
  for (int i = 0; i < node_num_; ++i) {
    std::cout << "节点" << i << " : " << x_coords[i] << std::endl;
  }
}

// ======================
// 接口函数
// getter 接口：外部获取网格数据（后面求解器要用）
int Mesh1D::get_node_num() const {
  return node_num_; 
}
// 获取数组指针（兼容后续计算）
std::vector<double>& Mesh1D::get_x_coords() {
  // vector 原生支持获取底层数组指针
  return x_coords;
}
const std::vector<double>& Mesh1D::get_x_coords() const { 
  return x_coords; 
}

  //获取参数（给IO模块使用）
double Mesh1D::get_start_x() const{
  return x_start_;
}
double Mesh1D::get_end_x() const{
  return x_end_;
}
