#include "python_module.hpp"

PYBIND11_MODULE(glaxnimate, glaxnimate_module)
{
    register_py_module(glaxnimate_module);
}
