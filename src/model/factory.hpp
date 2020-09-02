#pragma once

#include <unordered_map>
#include <memory>
#include <type_traits>

#include <QString>
#include <QHash>
#include <QMetaObject>


#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace std {
  template<> struct hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
      return (size_t) qHash(s);
    }
  };
}
#endif

namespace model {


class Object;
class Document;
class Composition;
class Layer;
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

} // namespace detail

class Factory
{
private:
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
        void register_type()
        {
            constructors.emplace(detail::naked_type_name<T>(), Builder::template for_type<T>());
        }

    private:
        std::unordered_map<QString, Builder> constructors;
    };

public:
    Object* make_object(const QString& class_name, Document* document) const
    {
        return object_factory.build(class_name, document);
    }

    Layer* make_layer(const QString& class_name, Document* document, Composition* comp) const
    {
        return layer_factory.build(class_name, document, comp);
    }

    Object* make_any(const QString& class_name, Document* document, Composition* comp) const;

    template<class Type>
    bool register_type()
    {
        if constexpr ( std::is_base_of_v<Layer, Type> )
            layer_factory.register_type<Type>();
        else
            object_factory.register_type<Type>();

        return true;
    }

    static Factory& instance()
    {
        static Factory instance;
        return instance;
    }

private:
    ~Factory() = default;
    Factory() = default;
    Factory(const Factory&) = delete;

    InternalFactory<Object, Document*> object_factory;
    InternalFactory<Layer, Document*, Composition*> layer_factory;
};

} // namespace model
