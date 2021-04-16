#pragma once

#include <QBrush>
#include <QPixmap>

#include "model/assets/asset.hpp"

namespace model {

class BrushStyle : public Asset
{
    Q_OBJECT

public:
    using User = ReferenceProperty<BrushStyle>;

    using Asset::Asset;

    QIcon instance_icon() const override;

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
