#pragma once

#include <QBrush>
#include <QPixmap>

#include "model/defs/def.hpp"

namespace model {

class BrushStyle : public Def
{
    Q_OBJECT

public:
    using User = ReferenceProperty<BrushStyle>;

    using Def::Def;

    QIcon reftarget_icon() const override;

    virtual QBrush brush_style(FrameTime t) const = 0;

signals:
    void style_changed();

protected:
    virtual void fill_icon(QPixmap& icon) const = 0;

    void invalidate_icon()
    {
        icon = {};
        emit style_changed();
    }

private:
    mutable QPixmap icon;
};

} // namespace model
