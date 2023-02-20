/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LOTTIEFILESSEARCHDIALOG_H
#define LOTTIEFILESSEARCHDIALOG_H

#include <memory>
#include <QDialog>

namespace glaxnimate::gui {

class LottieFilesSearchDialog : public QDialog
{
    Q_OBJECT

public:
    enum Code
    {
        Import = 2,
        Open = 3,
    };

    LottieFilesSearchDialog(QWidget* parent = nullptr);
    ~LottieFilesSearchDialog();

    const QUrl& selected_url() const;
    const QString& selected_name() const;

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void clicked_open();
    void clicked_import();
    void clicked_search();
    void clicked_next();
    void page_changed(int page);

private:
    class Private;
    std::unique_ptr<Private> d;
};


} // namespace glaxnimate::gui

#endif // LOTTIEFILESSEARCHDIALOG_H
