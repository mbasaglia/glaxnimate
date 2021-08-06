#include "property_delegate.hpp"

#include <QComboBox>
#include <QFontComboBox>
#include <QTimer>
#include <QApplication>
#include <QMetaEnum>
#include <QLineEdit>
#include <QCheckBox>

#include <QtColorWidgets/GradientEditor>
#include <QtColorWidgets/ColorSelector>

#include "item_models/property_model_base.hpp"
#include "widgets/spin2d.hpp"
#include "widgets/enum_combo.hpp"
#include "model/property/option_list_property.hpp"

using namespace glaxnimate::gui::style;
using namespace glaxnimate::gui;


void PropertyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant data = index.data();

    switch ( data.userType() )
    {
        case QMetaType::QPointF:
            return paint_xy<QPointF>(painter, option, index);
        case QMetaType::QVector2D:
        {
            auto value = index.data().value<QVector2D>();
            return paint_plaintext(
                QString("%1% x %2%")
                .arg(math::get(value, 0) * 100)
                .arg(math::get(value, 1) * 100),
                painter,
                option,
                index
            );
        }
        case QMetaType::QSizeF:
            return paint_xy<QSizeF>(painter, option, index);
    }


    if ( data.userType() == qMetaTypeId<QGradientStops>() )
    {
        QLinearGradient g(0, 0, 1, 0);
        g.setStops(data.value<QGradientStops>());
        g.setCoordinateMode(QGradient::ObjectMode);
        paintItem(painter, option, index, g);
        return;
    }

    return color_widgets::ColorDelegate::paint(painter, option, index);
}

QWidget* PropertyDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant data = index.data(Qt::EditRole);
    auto refprop = index.data(item_models::PropertyModelBase::ReferenceProperty);
    int prop_flags = index.data(item_models::PropertyModelBase::Flags).toInt();
    auto min = index.data(item_models::PropertyModelBase::MinValue);
    auto max = index.data(item_models::PropertyModelBase::MaxValue);

    if ( auto wid = create_editor_from_variant(data, prop_flags, parent, refprop, min, max) )
        return wid;

    return color_widgets::ColorDelegate::createEditor(parent, option, index);
}

static void combo_setup(QComboBox* combo)
{
    QTimer* cheeky = new QTimer(combo);
    QObject::connect(cheeky, &QTimer::timeout, combo, [combo]{
        combo->setFocus();
        combo->showPopup();
    });
    cheeky->setSingleShot(true);
    cheeky->start(qApp->doubleClickInterval() / 2);
}

void PropertyDelegate::setEditorData ( QWidget * editor, const QModelIndex & index ) const
{
    QVariant data = index.data(Qt::EditRole);
    auto refprop = index.data(item_models::PropertyModelBase::ReferenceProperty);
    int prop_flags = index.data(item_models::PropertyModelBase::Flags).toInt();

    if ( !set_editor_data(editor, data, prop_flags, refprop) )
        color_widgets::ColorDelegate::setEditorData(editor, index);
}

void PropertyDelegate::setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const
{
    QVariant data = index.data(Qt::EditRole);
    auto refprop = index.data(item_models::PropertyModelBase::ReferenceProperty);
    int prop_flags = index.data(item_models::PropertyModelBase::Flags).toInt();

    int status = -1;
    QVariant value = get_editor_data(editor, data, prop_flags, refprop, status);
    if ( status == 1 )
        model->setData(index, value);
    else if ( status == 0 )
        return color_widgets::ColorDelegate::setModelData(editor, model, index);
}

void PropertyDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QWidget* widget = option.widget;

    // Disable decoration
    auto data = index.data(Qt::EditRole);
    if ( (data.userType() >= QMetaType::User && data.canConvert<int>()) ||
        index.data(item_models::PropertyModelBase::ReferenceProperty).canConvert<model::ReferenceBase*>() )
    {
        opt.icon = QIcon();
        opt.features &= ~QStyleOptionViewItem::HasDecoration;
    }
    opt.showDecorationSelected = true;

    QStyle *style = widget ? widget->style() : QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    const int delta = editor->minimumWidth() - geom.width();
    if (delta > 0)
    {
        // we need to widen the geometry
        if (editor->layoutDirection() == Qt::RightToLeft)
            geom.adjust(-delta, 0, 0, 0);
        else
            geom.adjust(0, 0, delta, 0);
    }
    editor->setGeometry(geom);

    return;
    // color_widgets::ColorDelegate::updateEditorGeometry(editor, option, index);
}


void PropertyDelegate::paint_plaintext(const QString& text, QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text = text;
    const QWidget* widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
}

QSize style::PropertyDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto sh = color_widgets::ColorDelegate::sizeHint(option, index);
    if ( force_height )
        sh.setHeight(force_height);
    return sh;
}

QVariant PropertyDelegate::refprop(model::BaseProperty *prop, const model::PropertyTraits &traits) const
{
    if ( traits.flags & model::PropertyTraits::OptionList )
        return QVariant::fromValue(static_cast<model::OptionListPropertyBase*>(prop));
    else if ( traits.type == model::PropertyTraits::ObjectReference )
        return QVariant::fromValue((model::ReferenceBase*)static_cast<model::ReferencePropertyBase*>(prop));
    return {};
}

QWidget *PropertyDelegate::editor_from_property(model::BaseProperty *prop, QWidget *parent) const
{
    auto slot = [prop, this]{ set_property_data(static_cast<QWidget*>(sender()), prop); };
    auto traits = prop->traits();
    if ( traits.type == model::PropertyTraits::Color )
    {
        auto wid = new color_widgets::ColorSelector(parent);
        connect(wid, &color_widgets::ColorSelector::colorSelected, this, slot);
        wid->setMinimumSize(32, 32);
        return wid;
    }
    else if ( traits.type == model::PropertyTraits::Bool )
    {
        auto wid = new QCheckBox(parent);
        connect(wid, &QCheckBox::clicked, this, slot);
        return wid;
    }

    QVariant min;
    QVariant max;
    if ( traits.type == model::PropertyTraits::Float && (traits.flags & model::PropertyTraits::Animated) )
    {
        auto float_prop = static_cast<model::AnimatedProperty<float>*>(prop);
        min = float_prop->min();
        max = float_prop->max();
    }

    QWidget *editor = create_editor_from_variant(prop->value(), traits.flags, parent, refprop(prop, traits), min, max);
    if ( !editor )
    {
        auto wid = new QLineEdit(parent);
        connect(wid, &QLineEdit::textEdited, this, slot);
        return wid;
    }

    if ( auto combo = qobject_cast<QComboBox*>(editor) )
        connect(combo, QOverload<int>::of(&QComboBox::activated), this, slot);
    else if ( auto wid = qobject_cast<Spin2D*>(editor) )
        connect(wid, &Spin2D::value_changed, this, slot);
    else if ( auto wid = qobject_cast<QSpinBox*>(editor) )
        connect(wid, QOverload<int>::of(&QSpinBox::valueChanged), this, slot);
    else if ( auto wid = qobject_cast<QDoubleSpinBox*>(editor) )
        connect(wid, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, slot);

    return editor;

}

void PropertyDelegate::set_editor_data(QWidget *editor, model::BaseProperty *prop) const
{
    QSignalBlocker block(editor);

    auto traits = prop->traits();
    if ( traits.type == model::PropertyTraits::Color )
        return static_cast<color_widgets::ColorSelector*>(editor)->setColor(prop->value().value<QColor>());

    if ( traits.type == model::PropertyTraits::Bool )
        return static_cast<QCheckBox*>(editor)->setChecked(prop->value().toBool());

    if ( !set_editor_data(editor, prop->value(), traits.flags, refprop(prop, traits)) )
        static_cast<QLineEdit*>(editor)->setText(prop->value().toString());
}

bool PropertyDelegate::set_property_data(QWidget *editor, model::BaseProperty *prop) const
{
    auto traits = prop->traits();
    if ( traits.type == model::PropertyTraits::Color )
        return prop->set_undoable(static_cast<color_widgets::ColorSelector*>(editor)->color());
    if ( traits.type == model::PropertyTraits::Bool )
        return prop->set_undoable(static_cast<QCheckBox*>(editor)->isChecked());

    int status = -1;
    QVariant value = get_editor_data(editor, prop->value(), traits.flags, refprop(prop, traits), status);
    if ( status == 1 )
        return prop->set_undoable(value);
    if ( status == -1 )
        return false;
    return prop->set_undoable(static_cast<QLineEdit*>(editor)->text());
}

QWidget *PropertyDelegate::create_editor_from_variant(const QVariant &data, int prop_flags, QWidget *parent, const QVariant& refprop, const QVariant& min, const QVariant& max) const
{
    if ( refprop.canConvert<model::ReferenceBase*>() )
        return new QComboBox(parent);

    if ( prop_flags & model::PropertyTraits::OptionList )
    {
        if ( auto prop = refprop.value<model::OptionListPropertyBase*>() )
        {
            if ( prop->option_list_flags() & model::OptionListPropertyBase::FontCombo )
                return new QFontComboBox(parent);
        }

        return new QComboBox(parent);
    }

    if ( prop_flags & model::PropertyTraits::OptionList )
    {
        if ( auto prop = refprop.value<model::OptionListPropertyBase*>() )
        {
            if ( prop->option_list_flags() & model::OptionListPropertyBase::FontCombo )
                return new QFontComboBox(parent);
        }

        return new QComboBox(parent);
    }

    if ( data.userType() >= QMetaType::User && data.canConvert<int>() )
        return new EnumCombo(parent);

    switch ( data.userType() )
    {
        case QMetaType::QPointF:
            return new Spin2D(false, parent);
        case QMetaType::QVector2D:
            return new Spin2D(true, parent);
        case QMetaType::QSizeF:
            return new Spin2D(true, parent);
        case QMetaType::Float:
        case QMetaType::Double:
        {
            auto box = new SmallerSpinBox(false, parent);
            qreal mult = 1;
            if ( prop_flags & model::PropertyTraits::Percent )
            {
                mult = 100;
                box->setSuffix(tr("%"));
                box->setDecimals(0);
            }

            if ( min.isValid() )
            {
                double mind = min.toDouble();
                if ( mind != std::numeric_limits<double>::lowest() )
                    mind *= mult;
                box->setMinimum(mind);
            }

            if ( max.isValid() )
            {
                qreal maxf = max.toDouble();
                if ( maxf != std::numeric_limits<double>::max() )
                    maxf *= mult;
                box->setMaximum(maxf);
                if ( max == 1 )
                    box->setSingleStep(0.1);
            }
            return box;
        }
        case QMetaType::Int:
            return new SmallerSpinBoxInt(parent);
    }

    if ( data.userType() == qMetaTypeId<QGradientStops>() )
        return new color_widgets::GradientEditor(Qt::Horizontal, parent);

    return nullptr;
}

bool PropertyDelegate::set_editor_data(QWidget *editor, const QVariant &data, int prop_flags, const QVariant &refprop) const
{
    // Object reference combo
    if ( refprop.canConvert<model::ReferenceBase*>() )
    {
        if ( auto rpb = refprop.value<model::ReferenceBase*>() )
        {
            if ( QComboBox* combo = qobject_cast<QComboBox*>(editor) )
            {
                model::DocumentNode* current = rpb->get_ref();
                combo->clear();
                for ( model::DocumentNode* ptr : rpb->valid_options() )
                {
                    if ( ptr )
                        combo->addItem(QIcon(ptr->instance_icon()), ptr->object_name(), QVariant::fromValue(ptr));
                    else
                        combo->addItem("", QVariant::fromValue(ptr));

                    if ( ptr ==  current )
                        combo->setCurrentIndex(combo->count() - 1);
                }
                combo_setup(combo);
            }
        }
        return true;
    }
    // Option list reference combo
    else if ( prop_flags & model::PropertyTraits::OptionList )
    {
        if ( auto prop = refprop.value<model::OptionListPropertyBase*>() )
        {
            // Font combo
            if ( prop->option_list_flags() & model::OptionListPropertyBase::FontCombo )
            {
                QFontComboBox* fcombo = static_cast<QFontComboBox*>(editor);
                fcombo->setCurrentText(prop->value().toString());
                return true;
            };

            QComboBox* combo = static_cast<QComboBox*>(editor);
            combo->setEditable(true);

            if ( !(prop->option_list_flags() & model::OptionListPropertyBase::LaxValues) )
                combo->setInsertPolicy(QComboBox::NoInsert);

            auto current = prop->value();
            bool found = false;
            for ( const auto& item : prop->value_options() )
            {
                combo->addItem(item.toString(), item);
                if ( item == current )
                {
                    found = true;
                    combo->setCurrentIndex(combo->count() - 1);
                }
            }

            if ( !found && (prop->option_list_flags() & model::OptionListPropertyBase::LaxValues) )
            {
                combo->addItem(current.toString(), current);
                combo->setCurrentIndex(combo->count() - 1);
            }
        }

        return true;
    }
    // Enum combo
    else if ( data.userType() >= QMetaType::User && data.canConvert<int>() )
    {
        EnumCombo* combo = static_cast<EnumCombo*>(editor);
        combo->set_data_from_qvariant(data);
        return true;
    }
    // Cradient
    else if ( data.userType() == qMetaTypeId<QGradientStops>() )
    {
        static_cast<color_widgets::GradientEditor*>(editor)->setStops(data.value<QGradientStops>());
        return true;
    }

    // Spin boxes
    switch ( data.userType() )
    {
        case QMetaType::QPointF:
            static_cast<Spin2D*>(editor)->set_value(data.value<QPointF>());
            return true;
        case QMetaType::QVector2D:
            static_cast<Spin2D*>(editor)->set_value(data.value<QVector2D>());
            return true;
        case QMetaType::QSizeF:
            static_cast<Spin2D*>(editor)->set_value(data.value<QSizeF>());
            return true;
        case QMetaType::Float:
        case QMetaType::Double:
            static_cast<QDoubleSpinBox*>(editor)->setValue(data.toDouble() * ((prop_flags & model::PropertyTraits::Percent) ? 100 : 1));
            return true;
        case QMetaType::Int:
            static_cast<QSpinBox*>(editor)->setValue(data.toInt());
            return true;
    }

    return false;
}

QVariant PropertyDelegate::get_editor_data(QWidget *editor, const QVariant& data, int prop_flags, const QVariant &refprop, int& status) const
{
    status = 1;

    // Object reference combo
    if (
        (data.userType() >= QMetaType::User && data.canConvert<int>()) ||
        refprop.canConvert<model::ReferenceBase*>()
    )
    {
        QComboBox* combo = static_cast<QComboBox*>(editor);
        return combo->itemData(combo->currentIndex());
    }
    // Option list combo
    else if ( prop_flags & model::PropertyTraits::OptionList )
    {
        if ( auto prop = refprop.value<model::OptionListPropertyBase*>() )
        {
            // Font list
            if ( prop->option_list_flags() & model::OptionListPropertyBase::FontCombo )
            {
                QFontComboBox* combo = static_cast<QFontComboBox*>(editor);
                return combo->currentFont().family();
            }
            else
            {
                QComboBox* combo = static_cast<QComboBox*>(editor);
                if ( prop->option_list_flags() & model::OptionListPropertyBase::LaxValues )
                    return combo->currentText();
                else
                    return combo->currentData();
            }
        }
        status = -1;
        return {};
    }
    // Gradient
    else if ( data.userType() == qMetaTypeId<QGradientStops>() )
    {
        return QVariant::fromValue(static_cast<color_widgets::GradientEditor*>(editor)->stops());
    }

    // Spin boxes
    switch ( data.userType() )
    {
        case QMetaType::QPointF:
            return static_cast<Spin2D*>(editor)->value_point();
        case QMetaType::QVector2D:
            return static_cast<Spin2D*>(editor)->value_vector();
        case QMetaType::QSizeF:
            return static_cast<Spin2D*>(editor)->value_size();
        case QMetaType::Float:
        case QMetaType::Double:
            return static_cast<QDoubleSpinBox*>(editor)->value() / ((prop_flags & model::PropertyTraits::Percent) ? 100 : 1);
        case QMetaType::Int:
            return static_cast<QSpinBox*>(editor)->value();
    }

    status = 0;
    return {};
}
