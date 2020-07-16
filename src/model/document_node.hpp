#pragma once

#include <QList>
#include <QIcon>

#include "model/property.hpp"

namespace model {



class DocumentNode : public Object
{
    Q_OBJECT

public:
    Property<QString> name{this, "name", "nm", ""};

    bool docnode_visible() const { return visible_; }
    bool docnode_locked() const { return locked_; }

    virtual DocumentNode* docnode_parent() const = 0;
    virtual QIcon docnode_icon() const = 0;

    virtual int docnode_child_count() const = 0;
    virtual DocumentNode* docnode_child(int index) const = 0;

    QString docnode_name() const
    {
        return name.get().isEmpty() ? tr("Animation") : name.get();
    }

public slots:
    void docnode_set_visible(bool visible)
    {
        emit docnode_visible_changed(visible_ = visible);
    }

    void docnode_set_locked(bool locked)
    {
        emit docnode_locked_changed(locked_ = locked);
    }

signals:
    void docnode_child_add_begin(int row);
    void docnode_child_add_end();

    void docnode_child_remove_begin(int row);
    void docnode_child_remove_end();

    void docnode_visible_changed(bool) const;
    void docnode_locked_changed(bool) const;

private:
    bool visible_ = true;
    bool locked_ = false;
};


} // namespace model
