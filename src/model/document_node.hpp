#pragma once

#include <QList>
#include <QIcon>
#include <QUuid>


#include "model/property.hpp"

namespace model {

namespace graphics { class DocumentNodeGraphicsItem; }
class Document;

class DocumentNode : public Object
{
    Q_OBJECT

    class ChildRange;

    class ChildIterator
    {
    public:
        ChildIterator& operator++() noexcept { ++index; return *this; }
        DocumentNode* operator->() const { return parent->docnode_child(index); }
        DocumentNode* operator*() const { return parent->docnode_child(index); }
        bool operator==(const ChildIterator& oth) const noexcept
        {
            return parent == oth.parent && index == oth.index;
        }
        bool operator!=(const ChildIterator& oth) const noexcept
        {
            return !(*this == oth );
        }

    private:
        ChildIterator(const DocumentNode* parent, int index) noexcept : parent(parent), index(index) {}

        const DocumentNode* parent;
        int index;
        friend ChildRange;
    };


    class ChildRange
    {
    public:
        ChildIterator begin() const noexcept { return ChildIterator{parent, 0}; }
        ChildIterator end() const noexcept { return ChildIterator{parent, size()}; }
        int size() const { return parent->docnode_child_count(); }

    private:
        ChildRange(const DocumentNode* parent) noexcept : parent(parent) {}
        const DocumentNode* parent;
        friend DocumentNode;
    };


public:
    Property<QString> name{this, "name", ""};
    Property<QColor> group_color{this, "color", QColor{0, 0, 0, 0}};
    Property<QUuid> uuid{this, "uuid", {}, false};

    explicit DocumentNode(Document* document);

    bool docnode_visible() const { return visible_; }
    bool docnode_locked() const { return locked_; }

    virtual DocumentNode* docnode_parent() const = 0;
    virtual QIcon docnode_icon() const = 0;

    virtual int docnode_child_count() const = 0;
    virtual DocumentNode* docnode_child(int index) const = 0;

    virtual graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() = 0;

    QString object_name() const override;

    QString docnode_name() const;

    QColor docnode_group_color() const;

    Document* document() const { return document_; }

    ChildRange docnode_children() const noexcept { return ChildRange{this}; }

    template<class T=DocumentNode>
    T* docnode_find_by_uuid(const QUuid& uuid) const
    {
        for ( DocumentNode* child : docnode_children() )
        {
            if ( child->uuid.get() == uuid )
                return qobject_cast<T*>(child);
            if ( T* matched = child->docnode_find_by_uuid<T>(uuid) )
                return matched;
        }

        return nullptr;
    }

    template<class T=DocumentNode>
    T* docnode_find_by_name(const QString& name) const
    {
        for ( DocumentNode* child : docnode_children() )
        {
            if ( child->name.get() == name )
                return qobject_cast<T*>(child);
            if ( T* matched = child->docnode_find_by_name<T>(name) )
                return matched;
        }

        return nullptr;
    }

    template<class T=DocumentNode>
    std::vector<T*> docnode_find(const QString& type_name, bool include_self = true)
    {
        std::vector<T*> matches;
        const char* t_name = T::staticMetaObject.className();
        if ( include_self )
            docnode_find_impl_add(type_name, matches, t_name);
        docnode_find_impl(type_name, matches, t_name);
        return matches;
    }


    bool docnode_is_instance(const QString& type_name) const;

private:

    template<class T=DocumentNode>
    std::vector<T*> docnode_find_impl(const QString& type_name, std::vector<T*>& matches, const char* t_name)
    {
        for ( DocumentNode* child : docnode_children() )
        {
            child->docnode_find_impl_add(type_name, matches, t_name);
            child->docnode_find_impl(type_name, matches, t_name);
        }
    }
    template<class T=DocumentNode>
    void docnode_find_impl_add(const QString& type_name, std::vector<T*>& matches, const char* t_name)
    {
        if ( inherits(t_name) && docnode_is_instance(type_name) )
            matches.push_back(static_cast<T*>(this));
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
