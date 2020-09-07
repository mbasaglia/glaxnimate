#pragma once

#include <QtColorWidgets/ColorDelegate>

#include "math/vector.hpp"

namespace style {

class PropertyDelegate : public color_widgets::ColorDelegate
{
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData ( QWidget * editor, const QModelIndex & index ) const override;

    void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

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

    void paint_plaintext(const QString& text, QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

} // namespace style
