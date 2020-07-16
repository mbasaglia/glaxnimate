#pragma once

#include <QList>
#include <QIcon>

#include "model/property.hpp"

namespace model {

class DocumentNode : public Object
{
    Q_OBJECT

public:
    Property<QColor> group_color{this, "color", "__groupcolor", QColor{}};
    Property<QString> name{this, "name", "nm", ""};

    bool docnode_visible() const { return visible_; }
    bool docnode_locked() const { return locked_; }

    virtual DocumentNode* docnode_parent() const = 0;
    virtual QIcon docnode_icon() const = 0;

    virtual int docnode_child_count() const = 0;
    virtual DocumentNode* docnode_child(int index) const = 0;

    QString docnode_name() const
    {
        if ( !name.get().isEmpty() )
            return name.get();

        QString class_name = metaObject()->className();
        int ns = class_name.lastIndexOf(":");
        if ( ns != -1 )
            class_name = class_name.mid(ns+1);
        return class_name;
    }

    QColor docnode_group_color() const
    {
        QColor col = group_color.get();
        if ( !col.isValid() )
        {
            if ( auto parent = docnode_parent() )
                return parent->docnode_group_color();
            else
                return Qt::white;
        }
        return col;
    }

public slots:
    void docnode_set_name(const QString& name)
    {
        this->name.set(name);
        emit docnode_name_changed(name);
    }

    void docnode_set_group_color(const QColor& col)
    {
        group_color.set(col);
        emit docnode_group_color_changed(col);
    }

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
    void docnode_child_add_end(DocumentNode* node);

    void docnode_child_remove_begin(int row);
    void docnode_child_remove_end(DocumentNode* node);

    void docnode_visible_changed(bool);
    void docnode_locked_changed(bool);
    void docnode_name_changed(const QString&);
    void docnode_group_color_changed(const QColor&);

private:
    bool visible_ = true;
    bool locked_ = false;
};


} // namespace model
