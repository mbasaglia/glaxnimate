#pragma once

#include <QtTest/QtTest>
#include <QDebug>
#include "math/vector.hpp"

namespace QTest {
    template<>
    char* toString(const glaxnimate::math::Vec2& v)
    {
        return QTest::toString(
            "Vec2(" +
            QByteArray::number(v.x()) + ", " +
            QByteArray::number(v.y()) + ')'
        );
    }
}

QDebug operator<<(QDebug debug, const glaxnimate::math::Vec2 &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Vec2(" << c.x() << ", " << c.y() << ')';

    return debug;
}
