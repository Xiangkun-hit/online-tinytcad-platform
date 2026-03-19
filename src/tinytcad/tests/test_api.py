# 1. 添加编译好的模块路径
import sys
sys.path.append("../build/pythonapi")  # 关键：告诉Python去文件夹找模块 

# 2. 导入C++模块
import tinytcad_py # type: ignore

# ====================== 测试1：创建工具类 ======================
utils = tinytcad_py.Utils()
utils.init()
utils.log_info("Python 调用 tinytcad 成功！")

# ====================== 测试2：创建一维网格 ======================
# 20个节点，范围 0.0 ~ 1.0
mesh = tinytcad_py.Mesh1D(20, 0.0, 1.0)
mesh.print_mesh()

# ====================== 测试3：创建求解器 + 泊松模型 ======================
solver = tinytcad_py.Solver()
solver.set_possion_model()  
utils.log_info("开始求解泊松方程...")

# 调用求解
solver.solve(mesh.get_x_coords(), mesh.get_node_num())

# 打印结果
solver.print_result()

utils.log_success("Python 调用全部完成！")