/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <memory>

#include "widgets/dialogs/selection_manager.hpp"

namespace glaxnimate::android {

class MainWindow : public QMainWindow, public glaxnimate::gui::SelectionManager
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    model::Document* document() const override;
    QColor current_color() const override;
    void set_current_color(const QColor& c) override;
    QColor secondary_color() const override;
    void set_secondary_color(const QColor& c) override;
    QPen current_pen_style() const override;
    qreal current_zoom() const override;
    model::BrushStyle* linked_brush_style(bool secondary) const override;

    model::Composition* current_composition() const override;
    void set_current_composition(model::Composition* comp) override;

    model::VisualNode* current_document_node() const override;
    void set_current_document_node(model::VisualNode* node) override;

    void switch_tool(gui::tools::Tool* tool) override;

    QWidget* as_widget() override { return this; }

    std::vector<model::VisualNode*> cleaned_selection() const override;

    std::vector<io::mime::MimeSerializer*> supported_mimes() const override;
    void set_selection(const std::vector<model::VisualNode*>& selected) override;
    void update_selection(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected) override;

public slots:
    void open_intent(const QUrl& uri);

protected:
    void changeEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;

private slots:
    void tool_triggered(bool checked);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::android
#endif // MAIN_WINDOW_HPP
