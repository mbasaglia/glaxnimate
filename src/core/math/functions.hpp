#pragma once
#include <cmath>
#include <QtMath>

namespace math {

template<class T>
auto binom(T n, T k)
{
    return 1 / ( (n+1) * std::beta(n-k+1, k+1) );
}


} // namespace math
