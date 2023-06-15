#pragma once

#include <iterator>

namespace glaxnimate::utils {

/**
 * \brief CRTP that adds all the boilerplate of an iterator
 * wrapping a random-access iterator.
 *
 * All you need to do is overload operator->() and operator*().
 */
template<class Iterator, class BaseIterator>
class RandomAccessIteratorWrapper
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = typename BaseIterator::difference_type;

    // Iterator
    RandomAccessIteratorWrapper& operator++() { ++iter; return *this; }

    // Input/Output Iterator
    bool operator==(const RandomAccessIteratorWrapper& o) const { return iter == o.iter; }
    bool operator!=(const RandomAccessIteratorWrapper& o) const { return iter != o.iter; }
    Iterator operator++(int) { auto copy = *this; ++iter; return copy; }

    // Forward Iterator
    RandomAccessIteratorWrapper() = default;

    // Bidirectional Iterator
    Iterator& operator--() { --iter; return *cast_derived(); }
    Iterator operator--(int) { auto copy = *this; --iter; return copy; }

    // Random Access Iterator
    Iterator& operator+= (difference_type i) { iter += i; return *cast_derived(); }
    Iterator operator+ (difference_type i) const { return iter + i; }
    friend Iterator operator+ (difference_type i, const Iterator& iter) { return iter.iter + i; }
    Iterator& operator-= (difference_type i) { iter -= i; return *cast_derived(); }
    Iterator operator- (difference_type i) const { return iter - i; }
    Iterator operator- (const Iterator& o) const { return iter - o.iter; }
    bool operator<(const Iterator& o) const { return iter < o.iter; }
    bool operator<=(const Iterator& o) const { return iter <= o.iter; }
    bool operator>(const Iterator& o) const { return iter > o.iter; }
    bool operator>=(const Iterator& o) const { return iter >= o.iter; }

protected:
    using InternalIterator = BaseIterator;
    using Parent = RandomAccessIteratorWrapper;
    RandomAccessIteratorWrapper(BaseIterator iter) : iter(std::move(iter)) {}
    BaseIterator iter;

private:
    Iterator* cast_derived() { return static_cast<Iterator*>(this); }
    const Iterator* cast_derived() const { return static_cast<const Iterator*>(this); }
};
} // namespace glaxnimate::utils
