/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QHash>

#include <functional>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace std {
  template<> struct hash<QByteArray> {
    std::size_t operator()(const QByteArray& s) const noexcept {
      return (size_t) qHash(s);
    }
  };
}
#endif

