/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "python_module.hpp"

PYBIND11_MODULE(glaxnimate, glaxnimate_module)
{
    register_py_module(glaxnimate_module);
}
