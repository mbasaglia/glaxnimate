#pragma once

#include "bezier.hpp"

namespace glaxnimate::math::bezier {

class LengthData
{
public:
    struct SplitInfo
    {
        int index = 0;
        qreal ratio = 0;
        qreal length = 0;
        const LengthData* child = nullptr;

        SplitInfo descend() const
        {
            return child->at_ratio(ratio);
        }
    };

    explicit LengthData(const Solver& segment, int steps);

    explicit LengthData(const Bezier& bez, int steps);

    explicit LengthData(const MultiBezier& mbez, int steps);


    SplitInfo at_ratio(qreal ratio) const;

    SplitInfo at_length(qreal length) const;

    /**
     * \brief Returns the length such that
     *        `at_length(length).ratio == ratio`
     */
    qreal from_ratio(qreal ratio) const;

    qreal length() const noexcept;

    /**
     * \returns The length at which the child at \p index starts
     */
    qreal child_start(int index) const;

    /**
     * \returns The length at which the child at \p index ends
     */
    qreal child_end(int index) const;

private:
    LengthData(qreal t, qreal length, qreal cumulative_length);

    qreal t_ = 0;
    qreal length_ = 0;
    qreal cumulative_length_ = 0;
    std::vector<LengthData> children_;
    bool leaf_ = false;

};

} // namespace glaxnimate::math::bezier
