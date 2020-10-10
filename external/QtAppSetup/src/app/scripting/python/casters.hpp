#pragma once
#include <QString>
#include <QUuid>
#include <QVariant>
#include <QList>
#include <QVector>
#include <QMap>
#include <QHash>
#include <QByteArray>
#include <QDateTime>
#include <QDate>
#include <QTime>


#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <datetime.h>


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

template <> struct type_caster<QByteArray>
{
public:
    PYBIND11_TYPE_CASTER(QByteArray, _("QByteArray"));

    bool load(handle src, bool)
    {
        PyObject *source = src.ptr();
        if ( !PyBytes_Check(source) )
            return false;
        char* buffer;
        Py_ssize_t len;
        if ( PyBytes_AsStringAndSize(source, &buffer, &len) == -1)
            return false;
        value = QByteArray(buffer, len);
        return true;
    }

    static handle cast(const QByteArray& data, return_value_policy, handle)
    {
        PyObject * obj = PyBytes_FromStringAndSize(data.data(), data.size());
        if ( obj )
            return obj;
        return {};
    }
};

template <> struct type_caster<QDateTime>
{
public:
    PYBIND11_TYPE_CASTER(QDateTime, _("QDateTime"));

    bool load(handle src, bool)
    {
        if ( !PyDateTimeAPI ) PyDateTime_IMPORT;

        PyObject *source = src.ptr();
        if ( !PyDateTime_Check(source) )
            return false;

        value = QDateTime(
            QDate(
                PyDateTime_GET_YEAR(source),
                PyDateTime_GET_MONTH(source),
                PyDateTime_GET_DAY(source)
            ),
            QTime(
                PyDateTime_DATE_GET_HOUR(source),
                PyDateTime_DATE_GET_MINUTE(source),
                PyDateTime_DATE_GET_SECOND(source),
                PyDateTime_DATE_GET_MICROSECOND(source) / 1000
            )
        );

        return true;
    }

    static handle cast(QDateTime val, return_value_policy, handle)
    {
        if ( !PyDateTimeAPI ) PyDateTime_IMPORT;

        return PyDateTime_FromDateAndTime(
            val.date().year(), val.date().month(), val.date().day(),
            val.time().hour(), val.time().minute(), val.time().second(), val.time().msec() * 1000
        );
    }
};
template <> struct type_caster<QDate>
{
public:
    PYBIND11_TYPE_CASTER(QDate, _("QDate"));

    bool load(handle src, bool)
    {
        if ( !PyDateTimeAPI ) PyDateTime_IMPORT;

        PyObject *source = src.ptr();

        if ( !PyDate_Check(source) )
            return false;

        value = QDate(
            PyDateTime_GET_YEAR(source),
            PyDateTime_GET_MONTH(source),
            PyDateTime_GET_DAY(source)
        );

        return true;
    }

    static handle cast(QDate val, return_value_policy, handle)
    {
        if ( !PyDateTimeAPI ) PyDateTime_IMPORT;

        return PyDate_FromDate(
            val.year(), val.month(), val.day()
        );
    }
};
template <> struct type_caster<QTime>
{
public:
    PYBIND11_TYPE_CASTER(QTime, _("QTime"));

    bool load(handle src, bool)
    {
        if ( !PyDateTimeAPI ) PyDateTime_IMPORT;

        PyObject *source = src.ptr();
        if ( PyTime_Check(source) )
        {
            value = QTime(
                PyDateTime_TIME_GET_HOUR(source),
                PyDateTime_TIME_GET_MINUTE(source),
                PyDateTime_TIME_GET_SECOND(source),
                PyDateTime_TIME_GET_MICROSECOND(source) / 1000
            );

            return true;
        }
        if ( PyDateTime_Check(source) )
        {
            value = QTime(
                PyDateTime_DATE_GET_HOUR(source),
                PyDateTime_DATE_GET_MINUTE(source),
                PyDateTime_DATE_GET_SECOND(source),
                PyDateTime_DATE_GET_MICROSECOND(source) / 1000
            );

            return true;
        }
        return false;
    }

    static handle cast(QTime val, return_value_policy, handle)
    {
        if ( !PyDateTimeAPI ) PyDateTime_IMPORT;

        return PyTime_FromTime(
            val.hour(), val.minute(), val.second(), val.msec() * 1000
        );
    }
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





template <typename V1, typename V2> struct type_caster<QPair<V1,V2>> {
    using cast1 = make_caster<V1>;
    using cast2 = make_caster<V2>;
    using pair_t = QPair<V1,V2>;

    PYBIND11_TYPE_CASTER(pair_t, _("QPair[") + cast1::name + _(",") + cast2::name + _("]"));

    bool load(handle src, bool convert) {
        if (!isinstance<tuple>(src))
            return false;

        auto d = reinterpret_borrow<tuple>(src);
        if ( d.size() != 2 )
            return false;

        cast1 conv1;
        cast2 conv2;
        if ( !conv1.load(d[0].ptr(), convert) || !conv2.load(d[1].ptr(), convert) )
            return false;

        value.first = cast_op<V1&&>(std::move(conv1));
        value.second = cast_op<V2&&>(std::move(conv2));
        return true;
    }

    static handle cast(const pair_t& val, return_value_policy policy, handle parent)
    {
        tuple t(2);
        t[0] = reinterpret_steal<object>(cast1::cast(val.first, policy, parent));
        t[1] = reinterpret_steal<object>(cast2::cast(val.second, policy, parent));
        return t.release();
    }
};

} // namespace pybind11::detail


