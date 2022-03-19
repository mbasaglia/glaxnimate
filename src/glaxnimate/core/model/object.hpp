#pragma once

#include <memory>
#include <vector>

#include <QObject>
#include <QVariant>

#include "glaxnimate/core/model/animation/frame_time.hpp"
#include "glaxnimate/core/model/factory.hpp"

class QUndoCommand;


/**
 * \brief Sets up declarations of concrete Object sub-classes
 * \note Call GLAXNIMATE_OBJECT_IMPL for every class declared with GLAXNIMATE_OBJECT
 */
#define GLAXNIMATE_OBJECT(cls)                                          \
private:                                                                \
    Q_OBJECT                                                            \
    static bool _reg;                                                   \
    std::unique_ptr<Object> clone_impl() const override;                \
public:                                                                 \
    std::unique_ptr<cls> clone_covariant() const;                       \
    // macro end

/**
 * \brief Registers a class declared with GLAXNIMATE_OBJECT to be constructed
 * with model::Factory
 */
#define GLAXNIMATE_OBJECT_IMPL(cls)                                     \
    bool cls::_reg{glaxnimate::model::Factory::instance().register_type<cls>()}; \
    std::unique_ptr<cls> cls::clone_covariant() const                   \
    {                                                                   \
        auto object = std::make_unique<cls>(this->document());          \
        this->clone_into(object.get());                                 \
        return object;                                                  \
    }                                                                   \
    std::unique_ptr<glaxnimate::model::Object> cls::clone_impl() const  \
    {                                                                   \
        return clone_covariant();                                       \
    }                                                                   \
    // macro end

namespace glaxnimate::model {

class ObjectListPropertyBase;
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
    bool set(const QString& property, const QVariant& value);
    bool set_undoable(const QString& property, const QVariant& value);
    bool has(const QString& property) const;

    const std::vector<BaseProperty*>& properties() const;
    BaseProperty* get_property(const QString& property);

    virtual QString object_name() const { return type_name_human(); }
    virtual QString type_name_human() const { return tr("Unknown Object"); }
    virtual void set_time(FrameTime t);
    FrameTime time() const;

    QString type_name() const;

    Document* document() const;
    void transfer(Document* document);
    void push_command(QUndoCommand* cmd);

    template<class T> T* cast() { return qobject_cast<T*>(this); }
    template<class T> const T* cast() const { return qobject_cast<const T*>(this); }
    template<class T> bool is_instance() const { return metaObject()->inherits(&T::staticMetaObject); }

    virtual void stretch_time(qreal multiplier);

signals:
    void property_changed(const model::BaseProperty* prop, const QVariant& value);
    void visual_property_changed(const model::BaseProperty* prop, const QVariant& value);
    void removed();

protected:
    virtual void on_property_changed(const BaseProperty* prop, const QVariant& value)
    {
        Q_UNUSED(prop);
        Q_UNUSED(value);
    }
    void clone_into(Object* dest) const;
    virtual void on_transfer(model::Document* doc) {Q_UNUSED(doc)};

    class Autoreg
    {
    public:
        Autoreg(const QMetaObject&);
        Autoreg(const Autoreg&) = delete;
        Autoreg& operator=(const Autoreg&) = delete;
    };

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

} // namespace glaxnimate::model
