#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <memory>

#include "widgets/dialogs/document_environment.hpp"

class MainWindow : public QMainWindow, public glaxnimate::gui::DocumentEnvironment
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

    model::Composition* current_composition() override;
    void set_current_composition(model::Composition* comp) override;

    model::VisualNode* current_document_node() override;
    void set_current_document_node(model::VisualNode* node) override;

    void switch_tool(tools::Tool* tool) override;

    QWidget* as_widget() override { return this; }

    std::vector<model::VisualNode*> cleaned_selection() const override;

protected:
    void changeEvent(QEvent *e) override;

private slots:
    void tool_triggered(bool checked);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // MAIN_WINDOW_HPP