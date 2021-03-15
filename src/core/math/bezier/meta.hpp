#pragma once

#include "bezier.hpp"

QDataStream& operator<<(QDataStream& ds, const math::bezier::Point& p);
QDataStream& operator<<(QDataStream& ds, const math::bezier::Bezier& bez);
QDataStream& operator>>(QDataStream& ds, math::bezier::Point& p);
QDataStream& operator>>(QDataStream& ds, math::bezier::Bezier& bez);

namespace math::bezier {


void register_meta();

} // namespace math::bezier
