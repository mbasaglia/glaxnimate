#pragma once


#include <QAbstractItemModel>

#include "model/document.hpp"

namespace item_models {

class ProxyBase;

class DocumentModelBase : public QAbstractItemModel
{
public:
    using QAbstractItemModel::QAbstractItemModel;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    virtual model::DocumentNode* node(const QModelIndex& index) const = 0;
    virtual model::VisualNode* visual_node(const QModelIndex& index) const = 0;
    virtual QModelIndex node_index(model::DocumentNode* node) const = 0;
    virtual model::Document* document() const = 0;

protected:
    virtual std::pair<model::DocumentNode*, int> drop_position(const QModelIndex &parent, int row) const;

private:
    friend class ProxyBase;
};


} // namespace item_models

