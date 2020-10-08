#ifndef GRADIENTLISTWIDGET_H
#define GRADIENTLISTWIDGET_H

#include <memory>
#include <QWidget>

class GradientListWidget : public QWidget
{
    Q_OBJECT

public:
    GradientListWidget(QWidget* parent = nullptr);
    ~GradientListWidget();

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // GRADIENTLISTWIDGET_H
