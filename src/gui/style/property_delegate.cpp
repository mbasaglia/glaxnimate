#include "property_delegate.hpp"

#include <QComboBox>
#include <QFontComboBox>
#include <QTimer>
#include <QApplication>
#include <QMetaEnum>

#include <QtColorWidgets/GradientEditor>

#include "item_models/property_model.hpp"
#include "widgets/spin2d.hpp"
#include "widgets/enum_combo.hpp"
#include "model/property/option_list_property.hpp"

using namespace style;

void PropertyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant data = index.data();

    switch ( data.userType() )
    {
        case QMetaType::QPointF:
            return paint_xy<QPointF>(painter, option, index);
        case QMetaType::QVector2D:
            return paint_xy<QVector2D>(painter, option, index);
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
    auto refprop = index.data(item_models::PropertyModel::ReferenceProperty);

    if ( refprop.canConvert<model::ReferencePropertyBase*>() )
        return new QComboBox(parent);

    if ( index.data(item_models::PropertyModel::Flags).toInt() & model::PropertyTraits::OptionList )
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
            QVariant min = index.data(item_models::PropertyModel::MinValue);
            if ( min.isValid() )
                box->setMinimum(min.toDouble());
            QVariant max = index.data(item_models::PropertyModel::MaxValue);
            if ( max.isValid() )
            {
                qreal maxf = max.toDouble();
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
    auto refprop = index.data(item_models::PropertyModel::ReferenceProperty);

    // Object reference combo
    if ( refprop.canConvert<model::ReferencePropertyBase*>() )
    {
        if ( auto rpb = refprop.value<model::ReferencePropertyBase*>() )
        {
            if ( QComboBox* combo = qobject_cast<QComboBox*>(editor) )
            {
                model::DocumentNode* current = rpb->value().value<model::DocumentNode*>();
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
        return;
    }
    // Option list reference combo
    else if ( index.data(item_models::PropertyModel::Flags).toInt() & model::PropertyTraits::OptionList )
    {
        if ( auto prop = refprop.value<model::OptionListPropertyBase*>() )
        {
            // Font combo
            if ( prop->option_list_flags() & model::OptionListPropertyBase::FontCombo )
            {
                QFontComboBox* fcombo = static_cast<QFontComboBox*>(editor);
                fcombo->setCurrentText(prop->value().toString());
                return;
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

        return;
    }
    // Enum combo
    else if ( data.userType() >= QMetaType::User && data.canConvert<int>() )
    {
        EnumCombo* combo = static_cast<EnumCombo*>(editor);
        combo->set_data_from_qvariant(data);
        return;
    }
    // Cradient
    else if ( data.userType() == qMetaTypeId<QGradientStops>() )
    {
        static_cast<color_widgets::GradientEditor*>(editor)->setStops(data.value<QGradientStops>());
        return;
    }

    // Spin boxes
    switch ( data.userType() )
    {
        case QMetaType::QPointF:
            return static_cast<Spin2D*>(editor)->set_value(data.value<QPointF>());
        case QMetaType::QVector2D:
            return static_cast<Spin2D*>(editor)->set_value(data.value<QVector2D>());
        case QMetaType::QSizeF:
            return static_cast<Spin2D*>(editor)->set_value(data.value<QSizeF>());
        case QMetaType::Float:
        case QMetaType::Double:
            static_cast<QDoubleSpinBox*>(editor)->setValue(data.toDouble());
            return;
        case QMetaType::Int:
            static_cast<QSpinBox*>(editor)->setValue(data.toInt());
            return;
    }

    // Plain editor
    return color_widgets::ColorDelegate::setEditorData(editor, index);
}

void PropertyDelegate::setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const
{
    QVariant data = index.data(Qt::EditRole);
    auto refprop = index.data(item_models::PropertyModel::ReferenceProperty);

    // Object reference combo
    if (
        (data.userType() >= QMetaType::User && data.canConvert<int>()) ||
        refprop.canConvert<model::ReferencePropertyBase*>()
    )
    {
        QComboBox* combo = static_cast<QComboBox*>(editor);
        model->setData(index, combo->itemData(combo->currentIndex()));
        return;
    }
    // Option list combo
    else if ( index.data(item_models::PropertyModel::Flags).toInt() & model::PropertyTraits::OptionList )
    {
        if ( auto prop = refprop.value<model::OptionListPropertyBase*>() )
        {
            // Font list
            if ( prop->option_list_flags() & model::OptionListPropertyBase::FontCombo )
            {
                QFontComboBox* combo = static_cast<QFontComboBox*>(editor);
                model->setData(index, combo->currentFont().family());
            }
            else
            {
                QComboBox* combo = static_cast<QComboBox*>(editor);
                if ( prop->option_list_flags() & model::OptionListPropertyBase::LaxValues )
                    model->setData(index, combo->currentText());
                else
                    model->setData(index, combo->currentData());
            }
        }
        return;
    }
    // Gradient
    else if ( data.userType() == qMetaTypeId<QGradientStops>() )
    {
        model->setData(index, QVariant::fromValue(static_cast<color_widgets::GradientEditor*>(editor)->stops()));
        return;
    }

    // Spin boxes
    switch ( data.userType() )
    {
        case QMetaType::QPointF:
            model->setData(index, static_cast<Spin2D*>(editor)->value_point());
            return;
        case QMetaType::QVector2D:
            model->setData(index, static_cast<Spin2D*>(editor)->value_vector());
            return;
        case QMetaType::QSizeF:
            model->setData(index, static_cast<Spin2D*>(editor)->value_size());
            return;
        case QMetaType::Float:
        case QMetaType::Double:
            model->setData(index, static_cast<QDoubleSpinBox*>(editor)->value());
            return;
        case QMetaType::Int:
            model->setData(index, static_cast<QSpinBox*>(editor)->value());
            return;
    }

    // Plain editor
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
        index.data(item_models::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
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
