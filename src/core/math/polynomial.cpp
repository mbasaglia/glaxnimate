/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "polynomial.hpp"
#include "math.hpp"


namespace math = glaxnimate::math;


// Returns the real cube root of a value
static double cuberoot(double v)
{
    if ( v < 0 )
        return -math::pow(-v, 1./3);
    return math::pow(v, 1./3);
}

std::vector<double> glaxnimate::math::cubic_roots(double a, double b, double c, double d)
{

    // If a is 0, it's a quadratic
    if ( qFuzzyIsNull(a) )
        return quadratic_roots(b, c, d);

    // Cardano's algorithm.
    b /= a;
    c /= a;
    d /= a;

    double p = (3*c - b * b) / 3;
    double p3 = p / 3;
    double q = (2 * b*b*b - 9 * b * c + 27 * d) / 27;
    double q2 = q / 2;
    double discriminant = q2 * q2 + p3 * p3 * p3;

    // and some variables we're going to use later on:

    // 3 real roots:
    if ( discriminant < 0)
    {
        double mp3  = -p / 3;
        double r = math::sqrt(mp3*mp3*mp3);
        double t = -q / (2*r);
        double cosphi = t < -1 ? -1 : t > 1 ? 1 : t;
        double phi  = math::acos(cosphi);
        double crtr = cuberoot(r);
        double t1   = 2 * crtr;
        double root1 = t1 * math::cos(phi / 3) - b / 3;
        double root2 = t1 * math::cos((phi + 2 * math::pi) / 3) - b / 3;
        double root3 = t1 * math::cos((phi + 4 * math::pi) / 3) - b / 3;
        return {root1, root2, root3};
    }

    // 2 real roots
    if ( qFuzzyIsNull(discriminant) )
    {
        double u1 = q2 < 0 ? cuberoot(-q2) : -cuberoot(q2);
        double root1 = 2*u1 - b / 3;
        double root2 = -u1 - b / 3;
        return {root1, root2};
    }

    // 1 real root, 2 complex roots
    double sd = math::sqrt(discriminant);
    double u1 = cuberoot(sd - q2);
    double v1 = cuberoot(sd + q2);
    return {u1 - v1 - b / 3};
}

std::vector<double> glaxnimate::math::quadratic_roots(double a, double b, double c)
{
    // linear
    if ( qFuzzyIsNull(a) )
    {
        if ( qFuzzyIsNull(b) )
            return {};

        return {-c / b};
    }

    double s = b * b - 4 * a * c;

    // Complex roots
    if ( s < 0 )
        return {};

    double single_root = -b / (2 * a);

    // 1 root
    if ( qFuzzyIsNull(s) )
        return {single_root};

    double delta = math::sqrt(s) / (2 * a);

    // 2 roots
    return {single_root - delta, single_root + delta};
}
