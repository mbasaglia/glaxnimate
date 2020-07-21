#pragma once

#include <QList>
#include <QIcon>

#include "model/property.hpp"

namespace model {

namespace graphics { class DocumentNodeGraphicsItem; }
class Document;

class DocumentNode : public Object
{
    Q_OBJECT

public:
    Property<QString> name{this, "name", "nm", ""};
    Property<QColor> group_color{this, "color", "__groupcolor", QColor{0, 0, 0, 0}};

    explicit DocumentNode(Document* document);

    bool docnode_visible() const { return visible_; }
    bool docnode_locked() const { return locked_; }

    virtual DocumentNode* docnode_parent() const = 0;
    virtual QIcon docnode_icon() const = 0;

    virtual int docnode_child_count() const = 0;
    virtual DocumentNode* docnode_child(int index) const = 0;

    virtual graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() = 0;

    QString object_name() const override { return docnode_name(); }

    QString docnode_name() const;

    QColor docnode_group_color() const;

    Document* document() const { return document_; }

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
    void docnode_child_add_end(DocumentNode* node);

    void docnode_child_remove_begin(int row);
    void docnode_child_remove_end(DocumentNode* node);

    void docnode_visible_changed(bool);
    void docnode_locked_changed(bool);
    void docnode_name_changed(const QString&);
    void docnode_group_color_changed(const QColor&);

private slots:
    void on_value_changed(const QString& name, const QVariant&);

private:
    bool visible_ = true;
    bool locked_ = false;
    Document* document_;
};


/**
 * \brief Simple CRTP to help with the clone boilerplate
 */
template <class Derived, class Base>
class DocumentNodeBase : public Base
{
public:
    using Base::Base;

    std::unique_ptr<Derived> clone_covariant() const
    {
        auto object = std::make_unique<Derived>(this->document());
        this->clone_into(object.get());
        return object;
    }

protected:
    using Ctor = DocumentNodeBase;

private:
    std::unique_ptr<Object> clone_impl() const override
    {
        return clone_covariant();
    }
};


} // namespace model
