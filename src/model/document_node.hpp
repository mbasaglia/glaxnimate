#pragma once

#include <QList>
#include <QIcon>
#include <QUuid>


#include "model/property.hpp"

namespace model {

namespace graphics { class DocumentNodeGraphicsItem; }
class Document;
class ReferencePropertyBase;

class DocumentNode : public Object
{
    Q_OBJECT

    class ChildRange;

    using get_func_t = DocumentNode* (DocumentNode::*) (int) const;
    using count_func_t = int (DocumentNode::*) () const;

    class ChildIterator
    {
    public:

        ChildIterator& operator++() noexcept { ++index; return *this; }
        DocumentNode* operator->() const { return (parent->*get_func)(index); }
        DocumentNode* operator*() const { return (parent->*get_func)(index); }
        bool operator==(const ChildIterator& oth) const noexcept
        {
            return parent == oth.parent && index == oth.index;
        }
        bool operator!=(const ChildIterator& oth) const noexcept
        {
            return !(*this == oth );
        }

    private:
        ChildIterator(const DocumentNode* parent, int index, get_func_t get_func) noexcept
        : parent(parent), index(index), get_func(get_func) {}

        const DocumentNode* parent;
        int index;
        get_func_t get_func;
        friend ChildRange;
    };


    class ChildRange
    {
    public:
        ChildIterator begin() const noexcept { return ChildIterator{parent, 0, get_func}; }
        ChildIterator end() const noexcept { return ChildIterator{parent, size(), get_func}; }
        int size() const { return (parent->*count_func)(); }

    private:
        ChildRange(const DocumentNode* parent, get_func_t get_func, count_func_t count_func) noexcept
        : parent(parent), get_func(get_func), count_func(count_func) {}
        const DocumentNode* parent;
        get_func_t get_func;
        count_func_t count_func;
        friend DocumentNode;
    };


public:
    Property<QString> name{this, "name", ""};
    Property<QColor> group_color{this, "color", QColor{0, 0, 0, 0}};
    Property<QUuid> uuid{this, "uuid", {}, false};

    explicit DocumentNode(Document* document);

    bool docnode_visible() const { return visible_; }
    bool docnode_locked() const { return locked_; }

    virtual QIcon docnode_icon() const = 0;

    virtual DocumentNode* docnode_parent() const = 0;
    virtual int docnode_child_count() const = 0;
    virtual DocumentNode* docnode_child(int index) const = 0;

    virtual DocumentNode* docnode_group_parent() const { return docnode_parent(); }
    virtual int docnode_group_child_count() const { return docnode_child_count(); }
    virtual DocumentNode* docnode_group_child(int index) const { return docnode_child(index); }

    virtual graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() = 0;

    virtual std::vector<DocumentNode*> docnode_valid_references(const ReferencePropertyBase*) const { return {}; }
    virtual bool docnode_is_valid_reference(const ReferencePropertyBase* property, DocumentNode* node) const
    {
        for ( auto p : docnode_valid_references(property) )
            if ( p == node )
                return true;
        return false;
    }

    QString object_name() const override;

    QString docnode_name() const;

    QColor docnode_group_color() const;
    const QPixmap& docnode_group_icon() const;

    Document* document() const { return document_; }

    ChildRange docnode_children() const noexcept
    {
        return ChildRange{this, &DocumentNode::docnode_child, &DocumentNode::docnode_child_count};
    }
    ChildRange docnode_group_children() const noexcept
    {
        return ChildRange{this, &DocumentNode::docnode_group_child, &DocumentNode::docnode_group_child_count};
    }

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

protected:
    void docnode_on_update_group(bool force = false);
    void on_property_changed(const QString& name, const QVariant&) override;
    bool docnode_valid_color() const;


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

private:
    bool visible_ = true;
    bool locked_ = false;
    Document* document_;
    QPixmap group_icon{32, 32};
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



class ReferencePropertyBase : public BaseProperty
{
    Q_GADGET
public:
    ReferencePropertyBase(DocumentNode* obj, QString name, bool user_editable)
        : BaseProperty(obj, std::move(name), PropertyTraits{false, PropertyTraits::ObjectReference, user_editable}),
          parent(obj)
    {
    }

    std::vector<DocumentNode*> valid_options() const
    {
        return parent->docnode_valid_references(this);
    }

    bool is_valid_option(DocumentNode* ptr) const
    {
        return parent->docnode_is_valid_reference(this, ptr);
    }

    DocumentNode* validator() const
    {
        return parent;
    }

private:
    DocumentNode* parent;
};


template<class Type>
class ReferenceProperty : public ReferencePropertyBase
{
public:
    using held_type = Type*;

    ReferenceProperty(DocumentNode* obj, QString name, bool user_editable = true)
        : ReferencePropertyBase(obj, std::move(name), user_editable)
    {}

    void set(Type* value)
    {
        value_ = value;
        value_changed();
    }

    Type* get() const
    {
        return value_;
    }

    QVariant value() const override
    {
        if ( !value_ )
            return {};
        return QVariant::fromValue(value_);
    }

    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert(qMetaTypeId<held_type>()) )
            return false;
        QVariant converted = val;
        if ( !converted.convert(qMetaTypeId<held_type>()) )
            return false;
        Type* ptr = converted.value<held_type>();
        if ( !is_valid_option(ptr) )
            return false;
        set(ptr);
        return true;
    }

private:
    Type* value_ = nullptr;
};



} // namespace model
