#pragma once
#include "property.hpp"
#include "model/document_node.hpp"


#define GLAXNIMATE_PROPERTY_LIST_IMPL(type, name)           \
public:                                                     \
    QVariantList get_##name() const                         \
    {                                                       \
        QVariantList ret;                                   \
        for ( const auto & ptr : name )                     \
            ret.push_back(QVariant::fromValue(ptr.get()));  \
        return ret;                                         \
    }                                                       \
private:                                                    \
    Q_PROPERTY(QVariantList name READ get_##name)           \
    Q_CLASSINFO(#name, "property list " #type)              \
    // macro end

#define GLAXNIMATE_PROPERTY_LIST(type, name)                \
public:                                                     \
    ObjectListProperty<type> name{this, #name};             \
    GLAXNIMATE_PROPERTY_LIST_IMPL(type, name)               \
    // macro end


namespace glaxnimate::model {

class ObjectListPropertyBase : public BaseProperty
{
public:
    ObjectListPropertyBase(Object* obj, const QString& name)
        : BaseProperty(obj, name, {PropertyTraits::Object, PropertyTraits::List|PropertyTraits::Visual})
    {}

    /**
     * \brief Inserts a clone of the passed object
     * \return The internal object or \b nullptr in case of failure
     */
    virtual Object* insert_clone(Object* object, int index = -1) = 0;

    bool set_value(const QVariant& val) override
    {
        if ( !val.canConvert<QVariantList>() )
            return false;

        for ( const auto& v : val.toList() )
        {
            if ( !v.canConvert<Object*>() )
                continue;

            insert_clone(v.value<Object*>());
        }

        return true;
    }

    bool valid_value(const QVariant& val) const override
    {
        return val.canConvert<QVariantList>();
    }

    virtual std::vector<model::DocumentNode *> valid_reference_values(bool allow_null) const = 0;
    virtual bool is_valid_reference_value(model::DocumentNode *, bool allow_null) const = 0;

protected:
    void object_removed(model::DocumentNode* ptr)
    {
        ptr->removed_from_list();
    }

    void object_added(model::DocumentNode* ptr)
    {
        ptr->added_to_list(static_cast<DocumentNode*>(object()));
    }
};

namespace detail {
template<class Type>
class ObjectListProperty : public ObjectListPropertyBase
{
public:
    using value_type = Type;
    using pointer = std::unique_ptr<Type>;
    using reference = Type&;
//     using const_reference = const Type&;
    using iterator = typename std::vector<pointer>::const_iterator;

    /**
     * \brief Utility to perform raw operations on the list of objects
     * \warning Use with care, this won't invoke any callbacks
     */
    class Raw
    {
    public:
        using iterator = typename std::vector<pointer>::iterator;
        using move_iterator = std::move_iterator<iterator>;

        void clear()
        {
            subject->objects.clear();
        }
        iterator begin() { return subject->objects.begin(); }
        iterator end() { return subject->objects.end(); }
        move_iterator move_begin() { return move_iterator(begin()); }
        move_iterator move_end() { return move_iterator(end()); }

    private:
        friend ObjectListProperty;
        Raw(ObjectListProperty* subject) : subject(subject) {}
        ObjectListProperty* subject;
    };

    ObjectListProperty(
        Object* obj,
        const QString& name,
        PropertyCallback<void, Type*, int> callback_insert = &DocumentNode::docnode_child_add_end,
        PropertyCallback<void, Type*, int> callback_remove = &DocumentNode::docnode_child_remove_end,
        PropertyCallback<void, int> callback_insert_begin = &DocumentNode::docnode_child_add_begin,
        PropertyCallback<void, int> callback_remove_begin = &DocumentNode::docnode_child_remove_begin,
        PropertyCallback<void, int, int> callback_move_begin = &DocumentNode::docnode_child_move_begin,
        PropertyCallback<void, Type*, int, int> callback_move_end = &DocumentNode::docnode_child_move_end
    )
        : ObjectListPropertyBase(obj, name),
        callback_insert(std::move(callback_insert)),
        callback_remove(std::move(callback_remove)),
        callback_insert_begin(std::move(callback_insert_begin)),
        callback_remove_begin(std::move(callback_remove_begin)),
        callback_move_begin(std::move(callback_move_begin)),
        callback_move_end(std::move(callback_move_end))
    {}

    value_type* operator[](int i) const { return objects[i].get(); }
    int size() const { return objects.size(); }
    bool empty() const { return objects.empty(); }
    iterator begin() const { return objects.begin(); }
    iterator end() const { return objects.end(); }

    value_type* back() const
    {
        return objects.back().get();
    }


    value_type* emplace(value_type* p, int position = -1)
    {
        return insert(std::unique_ptr<value_type>(p), position);
    }

    value_type* insert(pointer p, int position = -1)
    {
        if ( !valid_index(position) )
            position = size();

        callback_insert_begin(this->object(), position);
        auto ptr = p.get();
        objects.insert(objects.begin()+position, std::move(p));
        ptr->set_time(object()->time());
        object_added(ptr);
        on_insert(position);
        callback_insert(this->object(), ptr, position);
        value_changed();
        return ptr;
    }

    bool valid_index(int index)
    {
        return index >= 0 && index < int(objects.size());
    }

    pointer remove(int index)
    {
        if ( !valid_index(index) )
            return {};
        callback_remove_begin(object(), index);
        auto it = objects.begin() + index;
        auto v = std::move(*it);
        objects.erase(it);
        object_removed(v.get());
        on_remove(index);
        callback_remove(object(), v.get(), index);
        value_changed();
        return v;
    }

    void move(int index_a, int index_b)
    {
        if ( index_b >= size() )
            index_b = size() - 1;

        if ( !valid_index(index_a) || !valid_index(index_b) || index_a == index_b )
            return;

        callback_move_begin(this->object(), index_a, index_b);

        auto moved = std::move(objects[index_a]);
        if ( index_a < index_b )
            std::move(objects.begin() + index_a + 1, objects.begin() + index_b + 1, objects.begin() + index_a);
        else
            std::move_backward(objects.begin() + index_b, objects.begin() + index_a, objects.begin() + index_a + 1);
        objects[index_b] = std::move(moved);

        on_move(index_a, index_b);
        callback_move_end(this->object(), objects[index_b].get(), index_a, index_b);
        value_changed();
    }

    QVariant value() const override
    {
        QVariantList list;
        for ( const auto& p : objects )
            list.append(QVariant::fromValue((Object*)p.get()));
        return list;
    }

    Object* insert_clone(Object* object, int index = -1) override
    {
        if ( !object )
            return nullptr;

        auto basep = object->clone();

        Type* casted = qobject_cast<Type*>(basep.get());
        if ( casted )
        {
            basep.release();
            insert(pointer(casted), index);
            return casted;
        }
        return nullptr;
    }

    void set_time(FrameTime t) override
    {
        for ( const auto& o : objects )
            o->set_time(t);
    }

    int index_of(value_type* obj, int not_found = -1) const
    {
        for ( int i = 0; i < size(); i++ )
            if ( objects[i].get() == obj )
                return i;
        return not_found;
    }

    /**
     * \brief Allows to perform raw operations on the elements
     * \warning Use with care, no callbacks will be invoked
     */
    Raw raw() { return Raw{this}; }

    void transfer(Document* doc) override
    {
        for ( const auto& obj : objects )
            obj->transfer(doc);
    }

    std::vector<model::DocumentNode *> valid_reference_values(bool allow_null) const override
    {
        std::vector<model::DocumentNode *> res;
        if ( allow_null )
        {
            res.reserve(objects.size() + 1);
            res.push_back(nullptr);
        }
        else
        {
            res.reserve(objects.size());
        }

        for ( const auto& c : objects )
            res.push_back(c.get());

        return res;
    }

    virtual bool is_valid_reference_value(model::DocumentNode * value, bool allow_null) const override
    {
        if ( !value )
            return allow_null;

        for ( const auto& c : objects )
            if ( c.get() == value )
                return true;
        return false;
    }

protected:
    virtual void on_insert(int index) { Q_UNUSED(index); }
    virtual void on_remove(int index) { Q_UNUSED(index); }
    virtual void on_move(int index_a, int index_b) { Q_UNUSED(index_a); Q_UNUSED(index_b); }

    std::vector<pointer> objects;
    PropertyCallback<void, Type*, int> callback_insert;
    PropertyCallback<void, Type*, int> callback_remove;
    PropertyCallback<void, int> callback_insert_begin;
    PropertyCallback<void, int> callback_remove_begin;
    PropertyCallback<void, int, int> callback_move_begin;
    PropertyCallback<void, Type*, int, int> callback_move_end;
};
} // namespace detail


template<class Type>
class ObjectListProperty : public detail::ObjectListProperty<Type>
{
public:
    using detail::ObjectListProperty<Type>::ObjectListProperty;
};

} // namespace glaxnimate::model
