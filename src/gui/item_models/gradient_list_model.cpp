#include "gradient_list_model.hpp"

void item_models::GradientListModel::set_defs(model::Defs* defs)
{
    beginResetModel();

    if ( this->defs )
    {
        disconnect(this->defs, nullptr, this, nullptr);
    }

    this->defs = defs;

    if ( defs )
    {
        connect(defs, &model::Defs::gradient_add_begin, this, &GradientListModel::on_add_begin);
        connect(defs, &model::Defs::gradient_add_end, this, &GradientListModel::on_add_end);
        connect(defs, &model::Defs::gradient_remove_begin, this, &GradientListModel::on_remove_begin);
        connect(defs, &model::Defs::gradient_remove_end, this, &GradientListModel::on_remove_end);
        connect(defs, &model::Defs::gradient_move_begin, this, &GradientListModel::on_move_begin);
        connect(defs, &model::Defs::gradient_move_end, this, &GradientListModel::on_move_end);
    }
    endResetModel();
}

void item_models::GradientListModel::on_add_begin(int i)
{
    beginInsertRows({}, i, i);
}

void item_models::GradientListModel::on_add_end(model::GradientColors* t)
{
    connect(t, &model::ReferenceTarget::name_changed, this, [this, t]{
        auto i = index(defs->gradient_colors.index_of(t), Columns::Name);
        dataChanged(i, i, {Qt::DisplayRole, Qt::EditRole});
    });
    connect(t, &model::GradientColors::colors_changed, this, [this, t]{
        auto i = index(defs->gradient_colors.index_of(t), Columns::Gradient);
        dataChanged(i, i, {Qt::DisplayRole, Qt::EditRole});
    });
    connect(t, &model::BrushStyle::users_changed, this, [this, t]{
        auto i = index(defs->gradient_colors.index_of(t), Columns::Users);
        dataChanged(i, i, {Qt::DisplayRole});
    });
    endInsertRows();
}

void item_models::GradientListModel::on_move_begin(int a, int b)
{
    beginMoveRows({}, a, a, {}, b);
}

void item_models::GradientListModel::on_move_end(model::GradientColors*, int, int)
{
    endMoveRows();
}

void item_models::GradientListModel::on_remove_begin(int i)
{
    beginRemoveRows({}, i, i);
}

void item_models::GradientListModel::on_remove_end(model::GradientColors* c)
{
    disconnect(c, nullptr, this, nullptr);
    endRemoveRows();
}

QVariant item_models::GradientListModel::data(const QModelIndex& index, int role) const
{
    if ( !defs || index.row() < 0 || index.row() >= defs->gradient_colors.size() )
        return {};

    auto item = defs->gradient_colors[index.row()];

    switch ( index.column() )
    {
        case Columns::Gradient:
            if ( role == Qt::EditRole || role == Qt::DisplayRole )
            {
                QLinearGradient g(QPointF(0, 0), QPointF(1, 0));
                g.setStops(item->colors.get());
                return QBrush(g);
            }
            else if ( role == Qt::SizeHintRole )
            {
                return QSize(128, 32);
            }
            break;
        case Columns::Name:
            switch ( role )
            {
                case Qt::EditRole:
                    return item->name.get();
                case Qt::DisplayRole:
                    return item->object_name();
            }
            break;
        case Columns::Users:
            if ( role == Qt::DisplayRole )
                return int(item->users().size());
            break;
    }

    return {};
}

bool item_models::GradientListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ( !defs || role != Qt::EditRole || index.row() < 0 || index.row() >= defs->gradient_colors.size() )
        return false;

    auto item = defs->gradient_colors[index.row()];

    switch ( index.column() )
    {
        case Columns::Name:
            return item->name.set_undoable(value);
        case Columns::Gradient:
        {
            QBrush b = value.value<QBrush>();
            if ( !b.gradient() )
                return false;
            return item->colors.set_undoable(QVariant::fromValue(b.gradient()->stops()));
        }
        default:
            return false;
    }
}

Qt::ItemFlags item_models::GradientListModel::flags(const QModelIndex& index) const
{
    auto flags = QAbstractTableModel::flags(index);
    if ( index.column() == 0 )
        flags |= Qt::ItemIsEditable;

    return flags;
}

int item_models::GradientListModel::rowCount(const QModelIndex&) const
{
    return defs ? defs->gradient_colors.size() : 0;
}

int item_models::GradientListModel::columnCount(const QModelIndex&) const
{
    return Columns::Count;
}

QVariant item_models::GradientListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Vertical )
        return {};

    switch ( section )
    {
        case Name:
            if ( role == Qt::DisplayRole || role == Qt::ToolTipRole )
                return tr("Name");
            break;
        case Gradient:
            if ( role == Qt::DisplayRole || role == Qt::ToolTipRole )
                return tr("Gradient");
            break;
        case Users:
            if ( role == Qt::DisplayRole )
                return tr("#");
            if ( role == Qt::ToolTipRole )
                return tr("Number of users");
            break;
    }
    return {};
}

model::GradientColors * item_models::GradientListModel::gradient(const QModelIndex& index) const
{
    if ( !index.isValid() || !defs )
        return nullptr;

    if ( index.row() < defs->gradient_colors.size() )
        return defs->gradient_colors[index.row()];

    return nullptr;
}

QModelIndex item_models::GradientListModel::gradient_to_index(model::GradientColors* gradient) const
{
    if ( !defs )
        return {};
    return createIndex(defs->gradient_colors.index_of(gradient), 0);
}
