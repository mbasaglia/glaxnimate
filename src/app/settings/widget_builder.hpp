#pragma once


#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>

#include "app/settings/setting.hpp"

namespace app::settings {


class WidgetBuilder
{
public:
    void add_widgets(const SettingList& settings_list, QWidget* parent, QFormLayout* layout, QVariantMap& target) const
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
            layout->addRow(label, wid);
        }
    }

private:
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
            wid->setMinimum(opt.min);
            wid->setMaximum(opt.max);
            wid->setValue(opt.get<int>(target));
            QObject::connect(wid, (void(QSpinBox::*)(int))&QSpinBox::valueChanged,
                             SettingSetter<int>{opt.slug, &target, opt.side_effects});
            return wid;
        }
        else if ( opt.type == Setting::Float )
        {
            auto wid = new QDoubleSpinBox();
            wid->setMinimum(opt.min);
            wid->setMaximum(opt.max);
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

} // namespace app::settings
