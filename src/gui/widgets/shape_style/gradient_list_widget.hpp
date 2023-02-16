#ifndef GRADIENTLISTWIDGET_H
#define GRADIENTLISTWIDGET_H

#include <memory>
#include <QWidget>

#include "widgets/dialogs/selection_manager.hpp"

namespace glaxnimate::model {
    class Document;
    class Fill;
    class Stroke;
    class BrushStyle;
    class Gradient;
} // namespace glaxnimate::model

namespace glaxnimate::gui {

class GradientListWidget : public QWidget
{
    Q_OBJECT

public:
    GradientListWidget(QWidget* parent = nullptr);
    ~GradientListWidget();

    void set_document(model::Document* doc);
    void set_targets(const std::vector<model::Fill*>& fills, const std::vector<model::Stroke*>& strokes);
    void set_current(model::Fill* fill, model::Stroke* stroke);
    void set_window(glaxnimate::gui::SelectionManager* window);

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void change_current_gradient();

signals:
    void selected(model::Gradient* gradient, bool secondary);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // GRADIENTLISTWIDGET_H
