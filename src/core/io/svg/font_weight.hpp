#pragma once
#include <array>
#include "math/vector.hpp"
#include "math/math.hpp"
namespace glaxnimate::io::svg::detail {

/**
 * Compare:
 *  https://doc.qt.io/qt-5/qfont.html#Weight-enum
 *  https://developer.mozilla.org/en-US/docs/Web/CSS/font-weight#common_weight_name_mapping
 */
struct WeightConverter
{
    inline static constexpr const std::array<int, 9> qt = {
        00,
        12,
        25,
        50,
        57,
        63,
        75,
        81,
        87,
    };
    inline static constexpr const std::array<int, 9> css = {
        100,
        200,
        300,
        400,
        500,
        600,
        700,
        900,
        950,
    };

    static int convert(int old, const std::array<int, 9>& from, const std::array<int, 9>& to)
    {
        int index;
        for ( index = 0; index < 9; index++ )
        {
            if ( from[index] == old )
                return to[index];
            else if ( from[index] > old )
                break;
        }

        if ( index == 9 )
            index--;

        qreal t = (old - from[index]) / qreal(from[index+1] - from[index]);
        return qRound(math::lerp<qreal>(to[index], to[index+1], t));
    }

};

} // namespace glaxnimate::io::svg::detail
