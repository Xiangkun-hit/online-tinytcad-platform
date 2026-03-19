#include "solver.h"
#include <iostream>
#include "../utils/utils.h"

void Solver::set_linear_model(){
  model_ = ModelType::LINEAR;
//   std::cout << "linear model" << std::endl;
}
void Solver::set_quadratic_model(){
  model_ = ModelType::QUADRATIC;
//   std::cout << "quadratic model" << std::endl;
}
void Solver::set_possion_model(){
  model_ = ModelType::POSSION;
}

// 核心求解函数：接收网格坐标 + 节点数量
void Solver::solve(const std::vector<double>& x_coords, int node_num) {
  // 1. 重置结果数组大小（和网格节点数一致）
  result_.resize(node_num);

  // 2. 遍历所有网格节点，执行计算
  // 示例：最简单的线性计算 result = 2 * x （可替换为任意TCAD物理模型）
  for (int i = 0; i < node_num; i++) {
    if (model_ == ModelType::LINEAR){
        result_[i] = 2.0 *x_coords[i];  // 线性模型（原模型）
    } 
    else if(model_ == ModelType::QUADRATIC){
        result_[i] = x_coords[i] *x_coords[i]; // 二次模型（新增）
    }
    else if(model_ == ModelType::POSSION){             // 一维泊松方程：d²φ/dx² = 1.0（标准测试源项）// 有限差分法 FDM + 追赶法求解三对角矩阵
        // 网格点数过少，直接返回
        if(node_num < 3){
            return;
        }

        double dx = x_coords[1] - x_coords[0];
        // 初始化电势数组
        potential_.resize(node_num, 0.0);

        // 三对角矩阵系数
        std::vector<double> a(node_num-1, -1.0);  // 下对角线
        std::vector<double> b(node_num,    2.0);  // 主对角线
        std::vector<double> c(node_num-1, -1.0);  // 上对角线
        std::vector<double> d(node_num, dx*dx * 1.0); // 右端项

        // 追赶法：前向消元
        for(int i = 1; i < node_num; ++i){
            double factor = a[i -1] / b[i - 1];
            b[i] -= factor * c[i-1];
            d[i] -= factor * d[i-1];
        }

        // 追赶法：回代求解
        potential_[node_num-1] = d[node_num-1] / b[node_num-1];
        for (int i = node_num-2; i >= 0; --i) {
            potential_[i] = (d[i] - c[i] * potential_[i+1]) / b[i];
        }
    }
  }
}




// 打印求解结果
void Solver::print_result() const {
  Utils utils;
  if(model_ == ModelType::LINEAR || model_ == ModelType::QUADRATIC){
    std::cout << "\n[Solver]数值求解完成！计算结果：" << std::endl;
    for (size_t i = 0; i < result_.size(); i++) {
    std::cout << " 节点 " << i << " : " << result_[i] << std::endl;
    }
  }
  else if(model_ == ModelType::POSSION){
    utils.log_info("\n[Solver]POSSION方程求解完成!计算结果：");
    int idx = 0;
    for(auto ite = potential_.begin(); ite != potential_.end(); ++ite,++idx){
        std::cout << " 节点 " << idx << " 电势φ: " << *ite << std::endl;
    }
  }
  
}

// 获取结果（对外接口）
const std::vector<double> &Solver::get_result() const { return result_; }

//静态校验函数：判断模型是否有效（全局通用）
bool Solver::is_valid_model(int model){
  // 判断数字是否在枚举范围内（0 ~ 最后一个模型）
  return model >=0 && model <= static_cast<int>(ModelType::POSSION);
}