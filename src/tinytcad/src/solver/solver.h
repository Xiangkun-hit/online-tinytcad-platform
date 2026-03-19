// src/solver/solver.h
// 求解器模块：
// TCAD微型仿真框架
#ifndef TINYTCAD_SOLVER_H
#define TINYTCAD_SOLVER_H

#include <vector>

// 计算模型枚举（线性 / 二次模型）
enum class ModelType{
LINEAR, QUADRATIC, POSSION
};

class Solver {
private:
  // 存储计算结果（自动内存管理，安全！）
  std::vector<double> result_;
  ModelType model_ = ModelType::LINEAR;// 默认线性模型
  std::vector<double> potential_;

public:
  Solver() = default;
  //禁用拷贝
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;

  //增加 移动构造+移动赋值
  Solver(Solver&&) = default;
  Solver& operator=(Solver&&) = default;
  ~Solver() = default;

  // 核心求解函数：接收网格坐标 + 节点数量
  void solve(const std::vector<double>& x_coords, int node_num);
  //切换模型
  void set_quadratic_model();
  void set_linear_model();
  void set_possion_model();
  
  
  // 打印求解结果
  void print_result() const;
  // 获取结果（对外接口）
  const std::vector<double> &get_result() const;
  const std::vector<double>& get_potential() const{return potential_;}
  ModelType get_model_type() const { return model_;}


  //静态校验函数：判断模型是否有效（全局通用）
  static bool is_valid_model(int model);
};

#endif