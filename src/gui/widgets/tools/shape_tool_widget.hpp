#ifndef SHAPETOOLWIDGET_H
#define SHAPETOOLWIDGET_H

#include <memory>

#include <QWidget>

class ShapeToolWidget : public QWidget
{
    Q_OBJECT

public:
    ShapeToolWidget(QWidget* parent=nullptr);
    ~ShapeToolWidget();

    bool create_group() const;
    bool create_fill() const;
    bool create_stroke() const;
    qreal stroke_width() const;

protected:
    void changeEvent ( QEvent* e ) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void check_checks();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // SHAPETOOLWIDGET_H
