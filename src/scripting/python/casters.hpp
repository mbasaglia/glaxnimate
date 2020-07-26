#pragma once
#include <QString>
#include <QUuid>
#include <QVariant>
#include <QList>
#include <QVector>
#include <QMap>
#include <QHash>


#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>


namespace pybind11::detail {
template <> struct type_caster<QString>
{
public:
    /**
    * This macro establishes the name 'QString' in
    * function signatures and declares a local variable
    * 'value' of type QString
    */
    PYBIND11_TYPE_CASTER(QString, _("QString"));

    /**
    * Conversion part 1 (Python->C++): convert a PyObject into a QString
    * instance or return false upon failure. The second argument
    * indicates whether implicit conversions should be applied.
    */
    bool load(handle src, bool ic)
    {
        type_caster<std::string> stdc;
        if ( stdc.load(src, ic) )
        {
            value = QString::fromStdString(stdc);
            return true;
        }
        return false;
    }

    /**
    * Conversion part 2 (C++ -> Python): convert an QString instance into
    * a Python object. The second and third arguments are used to
    * indicate the return value policy and parent object (for
    * ``return_value_policy::reference_internal``) and are generally
    * ignored by implicit casters.
    */
    static handle cast(QString src, return_value_policy policy, handle parent)
    {
        return type_caster<std::string>::cast(src.toStdString(), policy, parent);
    }
};

template <> struct type_caster<QUuid>
{
public:
    PYBIND11_TYPE_CASTER(QUuid, _("QUuid"));

    bool load(handle src, bool ic)
    {
        type_caster<QString> stdc;
        if ( stdc.load(src, ic) )
        {
            value = QUuid::fromString((const QString &)stdc);
            return true;
        }
        return false;
    }

    static handle cast(QUuid src, return_value_policy policy, handle parent)
    {
        return type_caster<QString>::cast(src.toString(), policy, parent);
    }
};



template <> struct type_caster<QVariant>
{
public:
    PYBIND11_TYPE_CASTER(QVariant, _("QVariant"));

    bool load(handle src, bool ic);

    static handle cast(QVariant, return_value_policy policy, handle parent);
};


template <typename Type> struct type_caster<QList<Type>> : list_caster<QList<Type>, Type> {};
template <typename Type> struct type_caster<QVector<Type>> : list_caster<QVector<Type>, Type> {};
template <> struct type_caster<QStringList> : list_caster<QStringList, QString> {};


template <typename Type, typename Key, typename Value> struct qt_map_caster {
    using key_conv   = make_caster<Key>;
    using value_conv = make_caster<Value>;

    bool load(handle src, bool convert) {
        if (!isinstance<dict>(src))
            return false;
        auto d = reinterpret_borrow<dict>(src);
        value.clear();
        for (auto it : d) {
            key_conv kconv;
            value_conv vconv;
            if (!kconv.load(it.first.ptr(), convert) ||
                !vconv.load(it.second.ptr(), convert))
                return false;
            value.insert(cast_op<Key &&>(std::move(kconv)), cast_op<Value &&>(std::move(vconv)));
        }
        return true;
    }

    template <typename T>
    static handle cast(T &&src, return_value_policy policy, handle parent) {
        dict d;
        return_value_policy policy_key = policy;
        return_value_policy policy_value = policy;
        if (!std::is_lvalue_reference<T>::value) {
            policy_key = return_value_policy_override<Key>::policy(policy_key);
            policy_value = return_value_policy_override<Value>::policy(policy_value);
        }
        for (auto it = src.begin(); it != src.end(); ++it )
        {
            auto key = reinterpret_steal<object>(key_conv::cast(forward_like<T>(it.key()), policy_key, parent));
            auto value = reinterpret_steal<object>(value_conv::cast(forward_like<T>(*it), policy_value, parent));
            if (!key || !value)
                return handle();
            d[key] = value;
        }
        return d.release();
    }

    PYBIND11_TYPE_CASTER(Type, _("Dict[") + key_conv::name + _(", ") + value_conv::name + _("]"));
};



template <typename Key, typename Value> struct type_caster<QMap<Key, Value>>
  : qt_map_caster<QMap<Key, Value>, Key, Value> { };
template <typename Key, typename Value> struct type_caster<QHash<Key, Value>>
  : qt_map_caster<QHash<Key, Value>, Key, Value> { };

} // namespace pybind11::detail


