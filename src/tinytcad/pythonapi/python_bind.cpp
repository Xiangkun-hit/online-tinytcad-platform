#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "mesh/mesh.h"
#include "solver/solver.h"
#include "utils/utils.h"

namespace py = pybind11;

// ==============================
// 模块名：tinytcad_py （和CMake完全一致）
// ==============================
PYBIND11_MODULE(tinytcad_py, m) {
    m.doc() = "tinytcad 一维数值仿真框架 Python 接口";

    // 绑定枚举
    py::enum_<ModelType>(m, "ModelType")
        .value("LINEAR", ModelType::LINEAR)
        .value("QUADRATIC", ModelType::QUADRATIC)
        .value("POSSION", ModelType::POSSION);


    // --------------------------
    // 绑定 Mesh1D
    // --------------------------
    py::class_<Mesh1D>(m, "Mesh1D")
        .def(py::init<int, double, double>())  // 构造函数
        .def("get_node_num", &Mesh1D::get_node_num)
        .def("get_x_coords", [](const Mesh1D& self) -> const std::vector<double>& {
            return self.get_x_coords();
        })
        .def("print_mesh", &Mesh1D::print_mesh);

    // --------------------------
    // 绑定 Solver
    // --------------------------
    py::class_<Solver>(m, "Solver")
        .def(py::init<>())
        .def("set_linear_model", &Solver::set_linear_model)
        .def("set_quadratic_model", &Solver::set_quadratic_model)
        .def("set_possion_model", &Solver::set_possion_model)
        .def("solve", &Solver::solve)
        .def("print_result", &Solver::print_result)
        .def("get_potential", &Solver::get_potential)
        .def("get_result", &Solver::get_result);

    // --------------------------
    // 绑定日志工具类
    // --------------------------
    py::class_<Utils>(m, "Utils")
        .def(py::init<>())
        .def("init", &Utils::init)
        .def("log_info", &Utils::log_info)
        .def("log_success", &Utils::log_success)
        .def("log_error", &Utils::log_error);
}