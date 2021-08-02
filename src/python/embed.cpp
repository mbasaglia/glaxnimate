#include <pybind11/embed.h>
#include "python_module.hpp"

PYBIND11_EMBEDDED_MODULE(glaxnimate, glaxnimate_module)
{
    register_py_module(glaxnimate_module);
}
