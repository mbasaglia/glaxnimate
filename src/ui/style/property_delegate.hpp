#pragma once
#include <QComboBox>
#include <QTimer>
#include <QApplication>
#include "QtColorWidgets/ColorDelegate"
#include "model/item_models/property_model.hpp"

class PropertyDelegate : public color_widgets::ColorDelegate
{
protected:
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if ( index.data(model::PropertyModel::ReferenceProperty).canConvert<model::ReferencePropertyBase*>() )
        {
            return new QComboBox(parent);
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
        return color_widgets::ColorDelegate::setModelData(editor, model, index);
    }

};
