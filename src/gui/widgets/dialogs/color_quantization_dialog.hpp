/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef COLORQUANTIZATIONDIALOG_H
#define COLORQUANTIZATIONDIALOG_H

#include <memory>
#include <QDialog>

namespace glaxnimate::gui {

class ColorQuantizationDialog : public QDialog
{
    Q_OBJECT

public:
    ColorQuantizationDialog(QWidget* parent = nullptr);
    ~ColorQuantizationDialog();

    std::vector<QRgb> quantize(const QImage& image, int k) const;



    void init_settings();
    void save_settings();
    void reset_settings();

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // COLORQUANTIZATIONDIALOG_H
