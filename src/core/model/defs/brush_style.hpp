#pragma once

#include <unordered_set>

#include <QBrush>
#include <QPainter>

#include "model/reference_target.hpp"
#include "utils/pseudo_mutex.hpp"
#include "model/property/reference_property.hpp"

namespace model {

class BrushStyle : public ReferenceTarget
{
    Q_OBJECT

public:
    using User = ReferenceProperty<BrushStyle>;

    using ReferenceTarget::ReferenceTarget;

    QIcon reftarget_icon() const override;

    virtual QBrush brush_style(FrameTime t) const = 0;

    const std::unordered_set<User*>& users() const;
    void add_user(User* user);
    void remove_user(User* user);

    void attach();
    void detach();


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
    std::unordered_set<User*> users_;
    utils::PseudoMutex detaching;
};

} // namespace model
