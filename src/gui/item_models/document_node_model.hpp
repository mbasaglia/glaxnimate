#pragma once


#include "document_model_base.hpp"

namespace item_models {


class DocumentNodeModel : public DocumentModelBase
{
public:
    enum ColumnTypes
    {
        ColumnColor = 0,
        ColumnName,
        ColumnVisible,
        ColumnLocked,
        ColumnUsers,

        ColumnCount
    };

    explicit DocumentNodeModel(QObject *parent = nullptr);
    ~DocumentNodeModel();

    int rowCount ( const QModelIndex & parent ) const override;
    int columnCount ( const QModelIndex & parent ) const override;
    bool moveRows ( const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild ) override;
    bool removeRows ( int row, int count, const QModelIndex & parent ) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data ( const QModelIndex & index, int role ) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags ( const QModelIndex & index ) const override;
    QModelIndex parent ( const QModelIndex & child ) const override;
    bool setData ( const QModelIndex & index, const QVariant & value, int role ) override;

    void clear_document();
    void set_document(model::Document* doc);
    model::DocumentNode* node(const QModelIndex& index) const override;
    model::VisualNode* visual_node(const QModelIndex& index) const override;
    QModelIndex node_index(model::DocumentNode* node) const override;
    model::Document* document() const override;

private:
    void connect_node(model::DocumentNode* node);
    void disconnect_node(model::DocumentNode* node);

    class Private;
    std::unique_ptr<Private> d;
};


} // namespace item_models
