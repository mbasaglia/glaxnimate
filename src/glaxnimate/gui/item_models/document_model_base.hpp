#pragma once


#include <QAbstractItemModel>

#include "glaxnimate/core/model/document.hpp"

namespace glaxnimate::gui::item_models {

class ProxyBase;

class DocumentModelBase : public QAbstractItemModel
{
public:
    using QAbstractItemModel::QAbstractItemModel;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    bool canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) const override;

    virtual model::DocumentNode* node(const QModelIndex& index) const = 0;
    virtual model::VisualNode* visual_node(const QModelIndex& index) const = 0;
    virtual QModelIndex node_index(model::DocumentNode* node) const = 0;
    virtual model::Document* document() const = 0;

protected:
    /**
     * \brief Returns the document node corresponding to \p parent and the insertion point for drop data
     */
    virtual std::pair<model::VisualNode*, int> drop_position(const QModelIndex &parent, int row, int column) const;

    /**
     * \brief Returns drop_position() if the data is well formed
     */
    std::tuple<model::VisualNode *, int, model::ShapeListProperty*>
        cleaned_drop_position(const QMimeData *data, Qt::DropAction action, const QModelIndex &parent, int row, int column) const;

private:
    friend class ProxyBase;
};


} // namespace glaxnimate::gui::item_models

