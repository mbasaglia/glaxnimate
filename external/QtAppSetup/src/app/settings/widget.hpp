#pragma once

#include <type_traits>

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>


#include "settings.hpp"

namespace app::settings {

namespace detail {

template<class WidgetType, class GetterT, GetterT getter, class SetterT, SetterT setter>
struct WidgetInfoImpl
{
    using value_type = decltype((std::declval<WidgetType*>()->*getter)());

    static auto get(const WidgetType* wid)
    {
        return (wid->*getter)();
    }

    template<class Arg>
    static void set(WidgetType* wid, Arg&& value)
    {
        (wid->*setter)(std::forward<Arg>(value));
    }
};

template<class WidgetType>
struct WidgetInfo;

#define WIDGET_DEF(Class, Getter, Setter) template<> struct WidgetInfo<Class> : public \
    WidgetInfoImpl<Class, decltype(&Class::Getter), &Class::Getter, decltype(&Class::Setter), &Class::Setter> {}

WIDGET_DEF(QSpinBox, value, setValue);
WIDGET_DEF(QDoubleSpinBox, value, setValue);
WIDGET_DEF(QLineEdit, text, setText);
WIDGET_DEF(QCheckBox, isChecked, setChecked);
WIDGET_DEF(QComboBox, currentIndex, setCurrentIndex);

#undef WIDGET_DEF

} // namespace detail

/**
 * \brief Utility to automatically save/restore widget state using settings
 */
template<class WidgetType>
class WidgetSetting
{
    using Info = detail::WidgetInfo<WidgetType>;
    using value_type = typename Info::value_type;

public:
    WidgetSetting(WidgetType* widget, const QString& group, const QString& prefix = {})
        : widget(widget), group(group), setting(prefix + widget->objectName())
    {}

    /// Defines the setting
    void define()
    {
        Info::set(widget, settings::define<value_type>(group, setting, Info::get(widget)));
    }

    /// Loads the setting and updates the widget
    void load()
    {
        Info::set(widget, settings::get<value_type>(group, setting, Info::get(widget)));
    }

    /// Saves the settings from the widget
    void save()
    {
        settings::set<value_type>(group, setting, Info::get(widget));
    }

    /// Resets the setting to its default
    void reset()
    {
        Info::set(widget, settings::get_default<value_type>(group, setting));
    }

private:
    WidgetType* widget;
    QString group;
    QString setting;
};

} // namespace app::settings
