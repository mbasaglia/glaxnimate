/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QtGlobal>
#include <QString>

#if QT_VERSION > QT_VERSION_CHECK(5, 15, 2)

#include <QStringView>

namespace utils {

inline QStringView left_ref(const QString& s, int n)
{
    return QStringView{s}.left(n);
}

inline QStringView right_ref(const QString& s, int n)
{
    return QStringView{s}.right(n);
}

inline QStringView mid_ref(const QString& s, int a, int b)
{
    return QStringView{s}.mid(a, b);
}

template<class... Args>
inline auto split_ref(const QString& s, Args&&... args)
{
    return QStringView{s}.split(std::forward<Args>(args)...);
}

using StringView = QStringView;

} // namespace utils

#else

#include <QStringRef>


namespace utils {

inline QStringRef left_ref(const QString& s, int n)
{
    return s.leftRef(n);
}

inline QStringRef right_ref(const QString& s, int n)
{
    return s.rightRef(n);
}

inline QStringRef mid_ref(const QString& s, int a, int b)
{
    return s.midRef(a, b);
}

template<class... Args>
inline auto split_ref(const QString& s, Args&&... args)
{
    return s.splitRef(std::forward<Args>(args)...);
}

using StringView = QStringRef;

} // namespace utils


#endif
