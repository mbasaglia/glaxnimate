/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vector>

namespace glaxnimate::math {


/**
 * \brief Returns the real roots of
 *      a x^3 + b x^2 + c x + d = 0
 */
std::vector<double> cubic_roots(double a, double b, double c, double d);

/**
 * \brief Returns the real roots of
 *      a x^2 + b x + c = 0
 */
std::vector<double> quadratic_roots(double a, double b, double c);

} // namespace glaxnimate::math
