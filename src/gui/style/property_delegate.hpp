/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QtColorWidgets/ColorDelegate>

#include "math/vector.hpp"
#include "model/property/property.hpp"

namespace glaxnimate::gui::style {

class PropertyDelegate : public color_widgets::ColorDelegate
{
public:
    void set_forced_height(int height) { force_height = height; }

    QWidget* editor_from_property(model::BaseProperty* prop, QWidget* parent) const;
    void set_editor_data(QWidget* editor, model::BaseProperty* prop) const;
    bool set_property_data(QWidget* editor, model::BaseProperty* prop) const;

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData ( QWidget * editor, const QModelIndex & index ) const override;

    void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

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

    QVariant refprop(model::BaseProperty *prop, const model::PropertyTraits& traits) const;
    QWidget* create_editor_from_variant(const QVariant &data, int prop_flags, QWidget *parent, const QVariant& refprop, const QVariant& min, const QVariant& max) const;
    bool set_editor_data(QWidget *editor, const QVariant &data, int prop_flags, const QVariant &refprop) const;
    QVariant get_editor_data(QWidget *editor, const QVariant &data, int prop_flags, const QVariant &refprop, int& status) const;

    int force_height = 0;
};

} // namespace glaxnimate::gui::style
