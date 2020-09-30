#ifndef COLORPICKERWIDGET_H
#define COLORPICKERWIDGET_H

#include <memory>
#include <QWidget>

class ColorPickerWidget : public QWidget
{
    Q_OBJECT

public:
    ColorPickerWidget ( QWidget* parent = nullptr );
    ~ColorPickerWidget();

    bool set_fill() const;
    bool set_stroke() const;
    void swap_fill_color();
    void set_color(const QColor& color);

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // COLORPICKERWIDGET_H
