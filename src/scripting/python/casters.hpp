#include <QString>
#include <QUuid>


#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>


namespace pybind11 { namespace detail {
    template <> struct type_caster<QString> {
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

    template <> struct type_caster<QUuid> {
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
}} // namespace pybind11::detail


