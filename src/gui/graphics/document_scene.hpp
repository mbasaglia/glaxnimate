/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QGraphicsScene>

#include "model/document.hpp"

namespace glaxnimate::gui::tools { class Tool; }

namespace glaxnimate::gui::graphics {

class DocumentNodeGraphicsItem;

class DocumentScene : public QGraphicsScene
{
    Q_OBJECT
public:
    enum SelectMode
    {
        Replace, ///< Replace current selection
        Append , ///< Append to current selection
        Toggle , ///< Toggles from selection
        Remove , ///< Removes from selection
    };

    DocumentScene();
    ~DocumentScene();

    void set_document(model::Document* document);
    void set_composition(model::Composition* comp);
    void clear_document() { set_document(nullptr); }
    void set_active_tool(tools::Tool* tool);
    model::Document* document() const;

    void add_selection(model::VisualNode* node);
    void remove_selection(model::VisualNode* node);
    void toggle_selection(model::VisualNode* node);
    void clear_selection();
    void user_select(const std::vector<model::VisualNode*>& selected, SelectMode flags);

    /// Currently selected nodes
    const std::vector<model::VisualNode*>& selection() const;
    /// Top-level items for selected nodes
    std::vector<model::VisualNode*> cleaned_selection();


    bool is_selected(model::VisualNode* node) const;
    bool is_descendant_of_selection(model::VisualNode* node) const;
    void show_editors(model::VisualNode* node);
    void show_custom_editor(model::VisualNode* node, std::unique_ptr<QGraphicsItem> editor);
    void hide_editors(model::VisualNode* node, bool recursive, bool if_not_selected);
    bool has_editors(model::VisualNode* node) const;
    QGraphicsItem* get_editor(model::VisualNode* node) const;

    model::VisualNode* item_to_node(const QGraphicsItem* item) const;
    DocumentNodeGraphicsItem* item_from_node(model::VisualNode* node) const;

    std::vector<DocumentNodeGraphicsItem*> nodes(const QPointF& point,      const QTransform& device_transform) const;
    std::vector<DocumentNodeGraphicsItem*> nodes(const QPainterPath& path,  const QTransform& device_transform, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    std::vector<DocumentNodeGraphicsItem*> nodes(const QPolygonF& path,     const QTransform& device_transform, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;

    void debug() const;

Q_SIGNALS:
    void node_user_selected(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected);
    void document_changed(model::Document* new_doc, model::Document* old_doc);
    void drawing_background(QPainter* painter, const QRectF& rect);

protected:
    void drawBackground(QPainter * painter, const QRectF & rect) override;
    void drawForeground(QPainter * painter, const QRectF & rect) override;

private Q_SLOTS:
    void connect_node(model::DocumentNode* node);
    void disconnect_node(model::DocumentNode* node);
    void move_node(model::DocumentNode* node, int from, int to);
    void node_locked(bool locked);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::graphics
