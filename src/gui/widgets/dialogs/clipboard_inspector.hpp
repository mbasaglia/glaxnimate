#ifndef CLIPBOARDINSPECTOR_H
#define CLIPBOARDINSPECTOR_H

#include <QDialog>

class QTabWidget;
class ClipboardInspector : public QDialog
{
    Q_OBJECT

public:
    ClipboardInspector(QWidget* parent = nullptr);

private:
    void load(QTabWidget* tab);

};

#endif // CLIPBOARDINSPECTOR_H
