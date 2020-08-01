#pragma once
#include <QComboBox>
#include <QTimer>
#include <QApplication>
#include "QtColorWidgets/ColorDelegate"
#include "model/item_models/property_model.hpp"

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
        }

        return color_widgets::ColorDelegate::paint(painter, option, index);
    }

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if ( index.data(model::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
        {
            return new QComboBox(parent);
        }

        switch ( index.data().userType() )
        {
            case QMetaType::QPointF:
                return make_editor_xy<QPointF>(index.data(), parent);
            case QMetaType::QVector2D:
                return make_editor_xy<QVector2D>(index.data(), parent);
            case QMetaType::Float:
            case QMetaType::Double:
                return make_spin(parent, index.data().toDouble(), "", false);
            case QMetaType::Int:
                return make_spin_int(parent, index.data().toInt(), "");
        }

        return color_widgets::ColorDelegate::createEditor(parent, option, index);
    }

    void setEditorData ( QWidget * editor, const QModelIndex & index ) const override
    {
        if ( index.data(model::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
        {
            if ( auto rpb = index.data(model::PropertyModel::ReferenceProperty).value<model::ReferencePropertyBase*>() )
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
                return set_editor_data_xy<QPointF>(index.data(), editor);
            case QMetaType::QVector2D:
                return set_editor_data_xy<QVector2D>(index.data(), editor);
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
        if ( index.data(model::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
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
                return set_model_data_xy<QPointF>(model, index, editor);
            case QMetaType::QVector2D:
                return set_model_data_xy<QVector2D>(model, index, editor);
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

        if ( index.data(model::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
        {
            // Disable decoration
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            opt.icon = QIcon();
            opt.features &= ~QStyleOptionViewItem::HasDecoration;
            opt.showDecorationSelected = true;
            const QWidget* widget = option.widget;
            QStyle *style = widget ? widget->style() : QApplication::style();
            QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
            const int delta = editor->minimumWidth() - geom.width();
            if (delta > 0)
            {
                //we need to widen the geometry
                if (editor->layoutDirection() == Qt::RightToLeft)
                    geom.adjust(-delta, 0, 0, 0);
                else
                    geom.adjust(0, 0, delta, 0);
            }
            editor->setGeometry(geom);
            return;
        }

        color_widgets::ColorDelegate::updateEditorGeometry(editor, option, index);
    }

private:
    template<class T>
    void paint_xy(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        T value = index.data().value<T>();
        paint_plaintext(QString("%1 x %2").arg(value.x()).arg(value.y()), painter, option, index);
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

    template<class T>
    void set_model_data_xy(QAbstractItemModel * model, const QModelIndex & index, QWidget* wid) const
    {
        model->setData(index, QVariant::fromValue(T(
            wid->findChild<QDoubleSpinBox*>("x")->value(),
            wid->findChild<QDoubleSpinBox*>("y")->value()
        )));
    }

    template<class T>
    void set_editor_data_xy(const QVariant& data, QWidget* wid) const
    {
        T value = data.value<T>();
        wid->findChild<QDoubleSpinBox*>("x")->setValue(value.x());
        wid->findChild<QDoubleSpinBox*>("y")->setValue(value.y());
    }

    template<class T>
    QWidget* make_editor_xy(const QVariant& data, QWidget* parent) const
    {
        T value = data.value<T>();
        QWidget* wid = new QWidget(parent);
        QHBoxLayout* lay = new QHBoxLayout(wid);
        wid->setLayout(lay);
        lay->addWidget(make_spin(wid, value.x(), "x", true));
        lay->addWidget(make_spin(wid, value.y(), "y", true));
        lay->setContentsMargins(0, 0, 0, 0);
        return wid;
    }

    QDoubleSpinBox* make_spin(QWidget* parent, double value, const QString& name, bool adaptive) const
    {
        QDoubleSpinBox* box = new QDoubleSpinBox(parent);
        box->setMinimum(-999'999.99);
        box->setMaximum(+999'999.99);
        box->setValue(value);
        box->setDecimals(2);
        box->setObjectName(name);
        box->setMaximumWidth(get_spin_size(box));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        if ( adaptive )
            box->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
#endif
        return box;
    }

    QSpinBox* make_spin_int(QWidget* parent, int value, const QString& name) const
    {
        QSpinBox* box = new QSpinBox(parent);
        box->setMinimum(-999'999);
        box->setMaximum(+999'999);
        box->setValue(value);
        box->setObjectName(name);
        box->setMaximumWidth(get_spin_size(box));
        return box;
    }

    int get_spin_size(QAbstractSpinBox* box) const
    {
        if ( spin_size == 0 )
        {
            const QFontMetrics fm(box->fontMetrics());
            QString s = "999.99";
            int w = qMax(0, fm.horizontalAdvance(s));
            w += 2; // cursor blinking space

            QStyleOptionSpinBox option;
            option.initFrom(box);
            QSize hint(w, box->height());
            option.subControls = QStyle::SC_SpinBoxEditField | QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
            option.frame = box->hasFrame();
            spin_size = box->style()->sizeFromContents(QStyle::CT_SpinBox, &option, hint, box).expandedTo(QApplication::globalStrut()).width();
        }

        return spin_size;
    }

    mutable int spin_size = 0;
};
