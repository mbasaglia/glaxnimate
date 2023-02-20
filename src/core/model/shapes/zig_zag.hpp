/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "path_modifier.hpp"

namespace glaxnimate::model {

class ZigZag : public StaticOverrides<ZigZag, PathModifier>
{
    GLAXNIMATE_OBJECT(ZigZag)
    GLAXNIMATE_ANIMATABLE(float, amplitude, 10)
    GLAXNIMATE_ANIMATABLE(float, frequency, 10, {}, 0)
public:
    enum Style
    {
        Saw = 1,
        Wave = 2
    };

    Q_ENUM(Style)
    GLAXNIMATE_PROPERTY(Style, style, Saw, nullptr, nullptr, PropertyTraits::Visual)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();

    math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const override;

protected:
    bool process_collected() const override;

};

} // namespace glaxnimate::model

