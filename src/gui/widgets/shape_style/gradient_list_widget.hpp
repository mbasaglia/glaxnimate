#ifndef GRADIENTLISTWIDGET_H
#define GRADIENTLISTWIDGET_H

#include <memory>
#include <QWidget>

#include "widgets/dialogs/document_environment.hpp"

namespace model {
    class Document;
    class Fill;
    class Stroke;
    class BrushStyle;
    class Gradient;
} // namespace model

class GradientListWidget : public QWidget
{
    Q_OBJECT

public:
    GradientListWidget(QWidget* parent = nullptr);
    ~GradientListWidget();

    void set_document(model::Document* doc);
    void set_targets(model::Fill* fill, model::Stroke* stroke);
    void set_window(glaxnimate::gui::DocumentEnvironment* window);

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

#endif // GRADIENTLISTWIDGET_H
