/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <type_traits>

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>

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
WIDGET_DEF(QRadioButton, isChecked, setChecked);
WIDGET_DEF(QComboBox, currentIndex, setCurrentIndex);
WIDGET_DEF(QPushButton, isChecked, setChecked);

#undef WIDGET_DEF

} // namespace detail

class WidgetSettingBase
{
public:
    virtual ~WidgetSettingBase() {}
    virtual void define() = 0;
    virtual void load() = 0;
    virtual void save() = 0;
    virtual void reset() = 0;
};

/**
 * \brief Utility to automatically save/restore widget state using settings
 */
template<class WidgetType>
class WidgetSetting : public WidgetSettingBase
{
    using Info = detail::WidgetInfo<WidgetType>;
    using value_type = typename Info::value_type;

public:
    WidgetSetting(WidgetType* widget, const QString& group, const QString& prefix = {})
        : widget(widget), group(group), setting(prefix + widget->objectName())
    {}

    /// Defines the setting
    void define() override
    {
        Info::set(widget, settings::define<value_type>(group, setting, Info::get(widget)));
    }

    /// Loads the setting and updates the widget
    void load() override
    {
        Info::set(widget, settings::get<value_type>(group, setting, Info::get(widget)));
    }

    /// Saves the settings from the widget
    void save() override
    {
        settings::set<value_type>(group, setting, Info::get(widget));
    }

    /// Resets the setting to its default
    void reset() override
    {
        Info::set(widget, settings::get_default<value_type>(group, setting));
    }

private:
    WidgetType* widget;
    QString group;
    QString setting;
};

class WidgetSettingGroup : public WidgetSettingBase
{
public:
    template<class WidgetType, class... Args>
    void add(WidgetType* widget, Args&&... args)
    {
        values.push_back(std::make_unique<WidgetSetting<WidgetType>>(widget, std::forward<Args>(args)...));
    }

    /// Defines the setting
    void define() override
    {
        for ( auto& setting : values )
            setting->define();
    }

    /// Loads the setting and updates the widget
    void load() override
    {
        for ( auto& setting : values )
            setting->load();
    }

    /// Saves the settings from the widget
    void save() override
    {
        for ( auto& setting : values )
            setting->save();
    }

    /// Resets the setting to its default
    void reset() override
    {
        for ( auto& setting : values )
            setting->reset();
    }

private:
    std::vector<std::unique_ptr<WidgetSettingBase>> values;
};

} // namespace app::settings
