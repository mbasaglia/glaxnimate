#ifndef CLIPBOARDINSPECTOR_H
#define CLIPBOARDINSPECTOR_H

#include <QDialog>

class QTabWidget;

namespace glaxnimate::gui {


class ClipboardInspector : public QDialog
{
    Q_OBJECT

public:
    ClipboardInspector(QWidget* parent = nullptr);

private:
    void load(QTabWidget* tab);

};


} // namespace glaxnimate::gui

#endif // CLIPBOARDINSPECTOR_H
