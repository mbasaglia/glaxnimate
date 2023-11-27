/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>

#ifndef WITHOUT_QT_COLOR_WIDGETS
#include "QtColorWidgets/ColorSelector"
#endif

#include "app/settings/setting.hpp"

#include <QEvent>
#include "app/settings/settings_group.hpp"
#include "app/utils/qstring_literal.hpp"

namespace app::settings {


class WidgetBuilder
{
public:
    void add_widgets(const SettingList& settings_list, QWidget* parent,
                     QFormLayout* layout, QVariantMap& target,
                     const QString& name_infix = {}) const
    {
        for ( const Setting& opt : settings_list )
        {
            if ( opt.type == Setting::Internal )
                continue;

            target[opt.slug] = opt.get_variant(target);
            QWidget* wid = make_setting_widget(opt, target);
            if ( !wid )
                continue;

            QLabel* label = new QLabel(opt.label, parent);
            label->setToolTip(opt.description);
            wid->setParent(parent);
            wid->setToolTip(opt.description);
            wid->setWhatsThis(opt.description);
            wid->setObjectName(object_name("widget"_qs, name_infix, opt.slug));
            label->setObjectName(object_name("label"_qs, name_infix, opt.slug));
            layout->addRow(label, wid);
        }
    }

    void translate_widgets(const SettingList& settings_list, QWidget* parent, const QString& name_infix = {})
    {
        for ( const Setting& opt : settings_list )
        {
            if ( opt.type == Setting::Internal )
                continue;

            if ( QWidget* wid = parent->findChild<QWidget*>(object_name("widget"_qs, name_infix, opt.slug)) )
            {
                wid->setToolTip(opt.description);
                wid->setWhatsThis(opt.description);
            }

            if ( QLabel* label = parent->findChild<QLabel*>(object_name("label"_qs, name_infix, opt.slug)) )
            {
                label->setToolTip(opt.description);
                label->setText(opt.label);
            }
        }
    }

    bool show_dialog(const SettingList& settings_list, QVariantMap& target,
                     const QString& title, QWidget* parent = nullptr)
    {
        QDialog dialog(parent);

        dialog.setWindowTitle(title);

        QFormLayout layout;
        dialog.setLayout(&layout);

        add_widgets(settings_list, &dialog, &layout, target);
        QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout.setWidget(layout.rowCount(), QFormLayout::SpanningRole, &box);
        QObject::connect(&box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(&box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if ( dialog.exec() == QDialog::Rejected )
            return false;

        return true;
    }

private:
    QString object_name(const QString& labwid, const QString& name_infix, const QString& slug) const
    {
        return "__settings_%1__%2%3"_qs.arg(labwid).arg(name_infix).arg(slug);
    }

    QWidget* make_setting_widget(const Setting& opt, QVariantMap& target) const
    {
        if ( !opt.choices.isEmpty() )
        {
            auto wid = new QComboBox();
            int index = 0;
            QVariant val = opt.get_variant(target);
            for ( const QString& key : opt.choices.keys() )
            {
                QVariant choice = opt.choices[key];
                wid->addItem(key, choice);
                if ( choice == val )
                    wid->setCurrentIndex(index);
                index++;
            }
            QObject::connect(wid, &QComboBox::currentTextChanged, [wid, slug=opt.slug, &target, side_effects=opt.side_effects](){
                target[slug] = wid->currentData();
                if ( side_effects )
                    side_effects(wid->currentData());
            });
            return wid;
        }
        else if ( opt.type == Setting::Info )
        {
            return new QLabel(opt.description);
        }
        else if ( opt.type == Setting::Bool )
        {
            auto wid = new QCheckBox();
            wid->setChecked(opt.get<bool>(target));
            QObject::connect(wid, &QCheckBox::toggled, SettingSetter<bool>{opt.slug, &target, opt.side_effects});
            return wid;
        }
        else if ( opt.type == Setting::Int )
        {
            auto wid = new QSpinBox();
            if ( opt.min == opt.max && opt.max == -1 )
            {
                wid->setMinimum(std::numeric_limits<int>::min());
                wid->setMaximum(std::numeric_limits<int>::max());
            }
            else
            {
                wid->setMinimum(opt.min);
                wid->setMaximum(opt.max);
            }
            wid->setValue(opt.get<int>(target));
            QObject::connect(wid, (void(QSpinBox::*)(int))&QSpinBox::valueChanged,
                             SettingSetter<int>{opt.slug, &target, opt.side_effects});
            return wid;
        }
        else if ( opt.type == Setting::Float )
        {
            auto wid = new QDoubleSpinBox();
            if ( opt.min == opt.max && opt.max == -1 )
            {
                wid->setMinimum(std::numeric_limits<double>::min());
                wid->setMaximum(std::numeric_limits<double>::max());
            }
            else
            {
                wid->setMinimum(opt.min);
                wid->setMaximum(opt.max);
            }
            wid->setValue(opt.get<float>(target));
            QObject::connect(wid, (void(QDoubleSpinBox::*)(double))&QDoubleSpinBox::valueChanged,
                             SettingSetter<float>{opt.slug, &target, opt.side_effects});
            return wid;
        }
        else if ( opt.type == Setting::String )
        {
            auto wid = new QLineEdit();
            wid->setText(opt.get<QString>(target));
            QObject::connect(wid, &QLineEdit::textChanged, SettingSetter<QString>{opt.slug, &target, opt.side_effects});
            return wid;
        }
#ifndef WITHOUT_QT_COLOR_WIDGETS
        else if ( opt.type == Setting::Color )
        {
            auto wid = new color_widgets::ColorSelector();
            wid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            wid->setColor(opt.get<QColor>(target));
            QObject::connect(wid, &color_widgets::ColorSelector::colorChanged, SettingSetter<QColor>{opt.slug, &target, opt.side_effects});
            return wid;
        }
#endif

        return nullptr;
    }

    template<class T>
    struct SettingSetter
    {
        QString slug;
        QVariantMap* options;
        std::function<void(const QVariant&)> side_effects;

        void operator()(T v)
        {
            if ( side_effects )
                side_effects(v);
            (*options)[slug] = QVariant(v);
        }
    };
};

class SettingsGroupWidget : public QWidget
{
public:
    SettingsGroupWidget(SettingsGroup* group, QWidget* parent = nullptr)
    : QWidget(parent),
      group(group)
    {
        QFormLayout* lay = new QFormLayout(this);
        this->setLayout(lay);
        bob.add_widgets(group->settings(), this, lay, group->values(), group->slug() + "__"_qs);
    }

    void changeEvent(QEvent *e) override
    {
        QWidget::changeEvent(e);

        if ( e->type() == QEvent::LanguageChange)
        {
            bob.translate_widgets(group->settings(), this, group->slug() + "__"_qs);
        }
    }

private:
    app::settings::WidgetBuilder bob;
    SettingsGroup* group;
};

} // namespace app::settings
