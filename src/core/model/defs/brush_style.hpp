#pragma once

#include <QBrush>
#include <QPainter>

#include "model/reference_target.hpp"

namespace model {

class BrushStyle : public ReferenceTarget
{
    Q_OBJECT

public:
    using ReferenceTarget::ReferenceTarget;

    QIcon reftarget_icon() const override;

    virtual QBrush brush_style(FrameTime t) const = 0;

signals:
    void style_changed();

protected:
    virtual void fill_icon(QPixmap& icon) const;

    void invalidate_icon()
    {
        icon = {};
        emit style_changed();
    }

private:
    mutable QPixmap icon;
};


} // namespace model
