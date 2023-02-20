/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <pybind11/embed.h>
#include "python_module.hpp"

PYBIND11_EMBEDDED_MODULE(glaxnimate, glaxnimate_module)
{
    register_py_module(glaxnimate_module);
}
