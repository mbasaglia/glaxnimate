#pragma once

#include <unordered_map>
#include <memory>
#include <type_traits>

#include <QMetaObject>

#include "utils/qstring_hash.hpp"

namespace model {


class Object;
class Document;
class Composition;
class ShapeElement;

namespace detail {

QString naked_type_name(QString class_name);

inline QString naked_type_name(const QMetaObject* obj)
{
    return naked_type_name(obj->className());
}

template<class T>
QString naked_type_name()
{
    return naked_type_name(&T::staticMetaObject);
}


template<class BaseType, class... Args>
class InternalFactory
{
private:
    class Builder
    {
    private:
        class Holder
        {
        public:
            virtual ~Holder() = default;
            virtual BaseType* construct(Args... args) const = 0;
        };

        template<class Type>
        class ConcreteHolder : public Holder
        {
        public:
            BaseType* construct(Args... args) const override
            {
                return new Type(args...);
            }
        };

    public:
        template<class Type>
        static Builder for_type()
        {
            return std::unique_ptr<Holder>(std::make_unique<ConcreteHolder<Type>>());
        }


        BaseType* construct(Args... args) const
        {
            return constructor->construct(args...);
        }

    private:
        Builder(std::unique_ptr<Holder> constructor)
        : constructor(std::move(constructor)) {}

        std::unique_ptr<Holder> constructor;
    };

public:
    BaseType* build(const QString& name, Args... args) const
    {
        auto it = constructors.find(name);
        if ( it == constructors.end() )
            return nullptr;
        return it->second.construct(args...);
    }

    template<class T>
    bool register_type()
    {
        constructors.emplace(detail::naked_type_name<T>(), Builder::template for_type<T>());
        return true;
    }

private:
    std::unordered_map<QString, Builder> constructors;
};

} // namespace detail

class Factory : public detail::InternalFactory<Object, Document*>
{
public:
    static Factory& instance()
    {
        static Factory instance;
        return instance;
    }

private:
    ~Factory() = default;
    Factory() = default;
    Factory(const Factory&) = delete;

};

} // namespace model
