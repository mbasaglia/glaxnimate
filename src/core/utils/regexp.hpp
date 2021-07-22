#pragma once
#include <QRegularExpression>


namespace glaxnimate::utils::regexp {

namespace detail {
    class EndIterator {};
    class NotDumbIterator
    {
    public:
        NotDumbIterator(QRegularExpressionMatchIterator dumb)
        : dumb(std::move(dumb))
        {
            ++*this;
        }

        NotDumbIterator& operator++()
        {
            end = !this->dumb.hasNext();
            if ( !end )
                match = this->dumb.next();
            return *this;
        }

        QRegularExpressionMatch& operator*() { return match; }

        bool operator!=(const NotDumbIterator& oth) const
        {
            return end != oth.end;
        };

        bool operator!=(const EndIterator&) const
        {
            return !end;
        }

    private:
        QRegularExpressionMatchIterator dumb;
        QRegularExpressionMatch match;
        bool end = false;
    };
} // namespace detail

struct MatchRange
{
    const detail::NotDumbIterator& begin() const { return begin_iter; }
    const detail::EndIterator& end() const { return end_iter; }

    detail::NotDumbIterator begin_iter;
    detail::EndIterator end_iter = {};
};

inline MatchRange find_all(const QRegularExpression& pattern, const QString& subject)
{
    return MatchRange{ detail::NotDumbIterator(pattern.globalMatch(subject)) };
}

} // namespace glaxnimate::utils::regexp
