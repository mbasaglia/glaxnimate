#pragma once

#include <QAbstractTableModel>

#include "model/defs/defs.hpp"

namespace item_models {

class GradientListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        Gradient,
        Name,
        Users,

        Count
    };

    void set_defs(model::Defs* defs);

    int rowCount(const QModelIndex & parent) const override;
    int columnCount(const QModelIndex & parent) const override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QModelIndex gradient_to_index(model::GradientColors* gradient) const;
    model::GradientColors* gradient(const QModelIndex& index) const;

private slots:
    void on_add_end(model::GradientColors*);
    void on_remove_end(model::GradientColors*);
    void on_add_begin(int);
    void on_remove_begin(int);
    void on_move_begin(int, int);
    void on_move_end(model::GradientColors*, int, int);

private:
    model::Defs* defs;
};

} // namespace item_models
