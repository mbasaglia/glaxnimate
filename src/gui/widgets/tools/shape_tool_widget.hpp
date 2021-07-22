#ifndef SHAPETOOLWIDGET_H
#define SHAPETOOLWIDGET_H

#include <memory>

#include <QWidget>

namespace glaxnimate::gui {

class ShapeToolWidget : public QWidget
{
    Q_OBJECT

public:
    ShapeToolWidget(QWidget* parent=nullptr);
    ~ShapeToolWidget();

    bool create_group() const;
    bool create_fill() const;
    bool create_stroke() const;
    bool create_layer() const;

private slots:
    void check_checks();

protected slots:
    void save_settings();

signals:
    void checks_changed();

protected:
    class Private;
    ShapeToolWidget(std::unique_ptr<Private> d, QWidget* parent);

    void changeEvent ( QEvent* e ) override;
    void showEvent(QShowEvent *event) override;

    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // SHAPETOOLWIDGET_H
