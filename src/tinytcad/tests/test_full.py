import sys
import os
sys.path.append("../build/pythonapi") 

import tinytcad_py as tinytcad # type: ignore
from tinytcad_py import ModelType # type: ignore

# 初始化
utils = tinytcad.Utils()
utils.init()

# 网格
mesh = tinytcad.Mesh1D(30, 0.0, 1.0)
x = mesh.get_x_coords()

# 测试1：泊松模型
utils.log_info("=== 测试泊松方程模型 ===")
solver = tinytcad.Solver()
solver.set_possion_model()
solver.solve(x, mesh.get_node_num())
solver.print_result()
phi = solver.get_potential()

# 测试2：线性模型
utils.log_info("=== 测试线性模型 ===")
solver2 = tinytcad.Solver()
solver2.set_linear_model()
solver2.solve(x, mesh.get_node_num())
solver2.print_result()

utils.log_success("全模型测试完成！")