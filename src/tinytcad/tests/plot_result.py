import sys
import os
sys.path.append("./build/pythonapi")
import tinytcad_py as tinytcad  # type: ignore
import matplotlib.pyplot as plt # type: ignore

# 计算
mesh = tinytcad.Mesh1D(50, 0, 1)
solver = tinytcad.Solver()
solver.set_possion_model()
solver.solve(mesh.get_x_coords(), mesh.get_node_num())

# 绘图
x = mesh.get_x_coords()
y = solver.get_potential()

plt.figure(figsize=(8,4))
plt.plot(x, y, "b-", linewidth=2, label="泊松方程电势")
plt.xlabel("X坐标")
plt.ylabel("电势 φ")
plt.title("tinytcad 一维泊松方程仿真结果")
plt.legend()
plt.grid(True)
plt.savefig("result.png")
plt.show()