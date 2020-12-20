#ifndef EDITTOOLWIDGET_H
#define EDITTOOLWIDGET_H

#include <memory>

#include <QWidget>

class EditToolWidget : public QWidget
{
    Q_OBJECT

public:
    EditToolWidget(QWidget* parent = nullptr);
    ~EditToolWidget();

    bool show_masks() const;

protected:
    void changeEvent ( QEvent* e ) override;

signals:
    void show_masks_changed(bool show);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // EDITTOOLWIDGET_H
