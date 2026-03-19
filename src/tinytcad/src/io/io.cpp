#include "io.h"
#include "../mesh/mesh.h"
#include "../solver/solver.h"
#include "../utils/utils.h"

#include <fstream>
#include <iostream>

// 核心：写入结果文件
void IO::write_file(const Mesh1D &mesh, const Solver &solver) {
  // 1. 创建文件（覆盖模式）
  std::ofstream file("result.txt");
  if (!file.is_open()) {
    std::cerr << "[IO ERROR] 文件打开失败！" << std::endl;
    return;
  }

  Utils utils;
  utils.log_info("[IO] 开始输出结果到 result.txt");

  // 2. 获取数据
  int node_num = mesh.get_node_num();
  const std::vector<double>& coords = mesh.get_x_coords();
  ModelType model_type = solver.get_model_type();

  if(model_type == ModelType::LINEAR || model_type == ModelType::QUADRATIC){
    // 3. 写入文件标题
    file << "==========================" << std::endl;
    file << "tinytcad 仿真结果文件（线性/二次模型）" << std::endl;
    file << "节点数量" << node_num << std::endl;
    file << "==========================" << std::endl;
    file << "节点编号\t坐标值\t\t计算结果" << std::endl;
    
    const auto &result = solver.get_result();
    // 4. 循环写入所有数据
    for (int i = 0; i < node_num; ++i) {
        file << i << "\t\t" << coords[i] << "\t\t" << result[i] << std::endl;
    }
  }
  else if(model_type == ModelType::POSSION){
    // 3. 写入文件标题
    file << "==========================" << std::endl;
    file << "tinytcad 仿真结果文件(POSSION模型)" << std::endl;
    file << "节点数量" << node_num << std::endl;
    file << "==========================" << std::endl;
    file << "节点编号\t坐标值\t\t计算结果" << std::endl;

    const auto &potential = solver.get_potential();
    // 4. 循环写入所有数据
    for (int i = 0; i < node_num; ++i) {
        file << i << "\t\t" << coords[i] << "\t\t" << potential[i] << std::endl;
    }
  }

  
  // 5. 关闭文件
  file.close();
  std::cout << "[IO] 结果已保存到 result.txt" << std::endl;
}