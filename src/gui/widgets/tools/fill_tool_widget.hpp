#ifndef FILLTOOLWIDGET_H
#define FILLTOOLWIDGET_H

#include <memory>
#include <QWidget>

class FillToolWidget : public QWidget
{
    Q_OBJECT

public:
    FillToolWidget(QWidget* parent = nullptr);
    ~FillToolWidget();

    bool fill() const;
    bool stroke() const;

    void swap_fill_color();

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // FILLTOOLWIDGET_H
