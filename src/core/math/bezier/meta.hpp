/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "bezier.hpp"

QDataStream& operator<<(QDataStream& ds, const glaxnimate::math::bezier::Point& p);
QDataStream& operator<<(QDataStream& ds, const glaxnimate::math::bezier::Bezier& bez);
QDataStream& operator>>(QDataStream& ds, glaxnimate::math::bezier::Point& p);
QDataStream& operator>>(QDataStream& ds, glaxnimate::math::bezier::Bezier& bez);

namespace glaxnimate::math::bezier {

void register_meta();

} // namespace glaxnimate::math::bezier
