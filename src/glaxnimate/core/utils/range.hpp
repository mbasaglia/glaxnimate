#pragma once

#include <iterator>

namespace glaxnimate::utils {


template<class Iterator, class Traits=Iterator>
class Range
{
public:
    using iterator = Iterator;
    using value_type = typename Traits::value_type;

    Range(iterator begin, iterator end)
        : begin_(begin), end_(end)
    {}

    iterator begin() const { return begin_; }
    iterator end() const { return end_; }
    iterator cbegin() const { return begin_; }
    iterator cend() const { return end_; }

private:
    iterator begin_;
    iterator end_;
};

} // namespace glaxnimate::utils
