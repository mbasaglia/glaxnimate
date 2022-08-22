#ifndef LOTTIEFILESSEARCHDIALOG_H
#define LOTTIEFILESSEARCHDIALOG_H

#include <memory>
#include <QDialog>

class LottieFilesSearchDialog : public QDialog
{
    Q_OBJECT

public:
    LottieFilesSearchDialog(QWidget* parent = nullptr);
    ~LottieFilesSearchDialog();

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // LOTTIEFILESSEARCHDIALOG_H
