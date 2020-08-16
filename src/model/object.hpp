#pragma once

#include <memory>
#include <vector>

#include <QObject>
#include <QVariant>

#include "model/animation/frame_time.hpp"

namespace model {

class BaseProperty;
class Document;

class Object : public QObject
{
    Q_OBJECT

public:
    explicit Object(Document* document);
    ~Object();

    std::unique_ptr<Object> clone() const
    {
        return clone_impl();
    }

    std::unique_ptr<Object> clone_covariant() const
    {
        auto object = std::make_unique<Object>(document());
        clone_into(object.get());
        return object;
    }

    virtual void assign_from(const Object* other);

    QVariant get(const QString& property) const;
    bool set(const QString& property, const QVariant& value, bool allow_unknown = false);
    bool set_undoable(const QString& property, const QVariant& value);
    bool has(const QString& property) const;

    const std::vector<BaseProperty*>& properties() const;
    BaseProperty* get_property(const QString& property);

    virtual QString object_name() const { return type_name_human(); }
    virtual QString type_name_human() const { return tr("Uknown Object"); }
    virtual void set_time(FrameTime t);

    QString type_name() const;

    Document* document() const;

    static QString naked_type_name(QString type_name);

signals:
    void property_added(const QString& name, const QVariant& value);
    void property_changed(const QString& name, const QVariant& value);

protected:
    virtual void on_property_changed(const BaseProperty* prop, const QVariant& value)
    {
        Q_UNUSED(prop);
        Q_UNUSED(value);
    }
    void clone_into(Object* dest) const;

private:
    virtual std::unique_ptr<Object> clone_impl() const
    {
        return clone_covariant();
    }

    void add_property(BaseProperty* prop);
    void property_value_changed(const BaseProperty* prop, const QVariant& value);

    friend BaseProperty;
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * \brief Simple CRTP to help with the clone boilerplate
 */
template <class Derived, class Base>
class ObjectBase : public Base
{
public:
    std::unique_ptr<Derived> clone_covariant() const
    {
        auto object = std::make_unique<Derived>(this->document());
        this->clone_into(object.get());
        return object;
    }

protected:
    using Base::Base;
    using Ctor = ObjectBase;

private:
    std::unique_ptr<Object> clone_impl() const override
    {
        return clone_covariant();
    }
};


} // namespace model
