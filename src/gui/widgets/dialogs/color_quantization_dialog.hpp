#ifndef COLORQUANTIZATIONDIALOG_H
#define COLORQUANTIZATIONDIALOG_H

#include <memory>
#include <QDialog>

class ColorQuantizationDialog : public QDialog
{
    Q_OBJECT

public:
    ColorQuantizationDialog(QWidget* parent = nullptr);
    ~ColorQuantizationDialog();

    std::vector<QRgb> quantize(const QImage& image, int k) const;

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // COLORQUANTIZATIONDIALOG_H
