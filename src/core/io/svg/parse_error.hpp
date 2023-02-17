#pragma once

#include <QString>
#include <exception>

namespace glaxnimate::io::svg {


class SvgParseError : public std::exception
{
public:
    QString formatted(const QString& filename) const
    {
        return QString("%1:%2:%3: SVG Parse Error: %4")
            .arg(filename)
            .arg(line)
            .arg(column)
            .arg(message)
        ;
    }

    QString message;
    int line = -1;
    int column = -1;
};

} // namespace glaxnimate::io::svg
