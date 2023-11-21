/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QBrush>
#include <QPixmap>

#include "model/assets/asset.hpp"

namespace glaxnimate::model {

class BrushStyle : public Asset
{
    Q_OBJECT

public:
    using User = ReferenceProperty<BrushStyle>;

    using Asset::Asset;

    QIcon instance_icon() const override;

    virtual QBrush brush_style(FrameTime t) const = 0;
    virtual QBrush constrained_brush_style(FrameTime t, const QRectF& bounds) const;

Q_SIGNALS:
    void style_changed();

protected:
    virtual void fill_icon(QPixmap& icon) const = 0;

    void invalidate_icon()
    {
        icon = {};
        Q_EMIT style_changed();
    }

private:
    mutable QPixmap icon;
};

} // namespace glaxnimate::model
