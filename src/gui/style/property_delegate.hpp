#pragma once
#include <QComboBox>
#include <QTimer>
#include <QApplication>
#include "QtColorWidgets/ColorDelegate"
#include "item_models/property_model.hpp"
#include "widgets/spin2d.hpp"
#include "math/vector.hpp"

class PropertyDelegate : public color_widgets::ColorDelegate
{
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        switch ( index.data().userType() )
        {
            case QMetaType::QPointF:
                return paint_xy<QPointF>(painter, option, index);
            case QMetaType::QVector2D:
                return paint_xy<QVector2D>(painter, option, index);
            case QMetaType::QSizeF:
                return paint_xy<QSizeF>(painter, option, index);
        }

        return color_widgets::ColorDelegate::paint(painter, option, index);
    }

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if ( index.data(item_models::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
        {
            return new QComboBox(parent);
        }

        switch ( index.data().userType() )
        {
            case QMetaType::QPointF:
                return new Spin2D(false, parent);
            case QMetaType::QVector2D:
                return new Spin2D(true, parent);
            case QMetaType::QSizeF:
                return new Spin2D(true, parent);
            case QMetaType::Float:
            case QMetaType::Double:
                return new SmallerSpinBox(false, parent);
            case QMetaType::Int:
                return new SmallerSpinBoxInt(parent);
        }

        return color_widgets::ColorDelegate::createEditor(parent, option, index);
    }

    void setEditorData ( QWidget * editor, const QModelIndex & index ) const override
    {
        if ( index.data(item_models::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
        {
            if ( auto rpb = index.data(item_models::PropertyModel::ReferenceProperty).value<model::ReferencePropertyBase*>() )
            {
                if ( QComboBox* combo = qobject_cast<QComboBox*>(editor) )
                {
                    model::DocumentNode* current = rpb->value().value<model::DocumentNode*>();
                    combo->clear();
                    for ( model::DocumentNode* ptr : rpb->valid_options() )
                    {
                        if ( ptr )
                            combo->addItem(QIcon(ptr->docnode_group_icon()), ptr->docnode_name(), QVariant::fromValue(ptr));
                        else
                            combo->addItem("", QVariant::fromValue(ptr));

                        if ( ptr ==  current )
                            combo->setCurrentIndex(combo->count() - 1);
                    }

                    QTimer* cheeky = new QTimer(combo);
                    connect(cheeky, &QTimer::timeout, combo, [combo]{
                        combo->setFocus();
                        combo->showPopup();
                    });
                    cheeky->setSingleShot(true);
                    cheeky->start(qApp->doubleClickInterval() / 2);
                }
            }
            return;
        }

        switch ( index.data().userType() )
        {
            case QMetaType::QPointF:
                return static_cast<Spin2D*>(editor)->set_value(index.data().value<QPointF>());
            case QMetaType::QVector2D:
                return static_cast<Spin2D*>(editor)->set_value(index.data().value<QVector2D>());
            case QMetaType::QSizeF:
                return static_cast<Spin2D*>(editor)->set_value(index.data().value<QSizeF>());
            case QMetaType::Float:
            case QMetaType::Double:
                static_cast<QDoubleSpinBox*>(editor)->setValue(index.data().toDouble());
                return;
            case QMetaType::Int:
                static_cast<QSpinBox*>(editor)->setValue(index.data().toInt());
                return;
        }

        return color_widgets::ColorDelegate::setEditorData(editor, index);
    }

    void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const override
    {
        if ( index.data(item_models::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
        {
            if ( QComboBox* combo = qobject_cast<QComboBox*>(editor) )
            {
                model->setData(index, combo->itemData(combo->currentIndex()));
            }
            return;
        }

        switch ( index.data().userType() )
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

        return color_widgets::ColorDelegate::setModelData(editor, model, index);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        const QWidget* widget = option.widget;

        // Disable decoration
        if ( index.data(item_models::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
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

private:
    template<class T>
    void paint_xy(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        T value = index.data().value<T>();
        paint_plaintext(
            QString("%1 x %2")
            .arg(math::get(value, 0))
            .arg(math::get(value, 1)),
            painter,
            option,
            index
        );
    }

    void paint_plaintext(const QString& text, QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text = text;
        const QWidget* widget = option.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }
};
