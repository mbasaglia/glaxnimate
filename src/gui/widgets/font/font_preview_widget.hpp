/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FONT_FONTPREVIEWWIDGET_H
#define FONT_FONTPREVIEWWIDGET_H

#include <memory>
#include <QWidget>

namespace glaxnimate::gui::font {

class FontPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    FontPreviewWidget(QWidget* parent = nullptr);
    ~FontPreviewWidget();

    void set_text(const QString& text, bool fallback_to_default = true);

    QFont selected_font() const;

public Q_SLOTS:
    void set_font(const QFont& font);

protected:
    void changeEvent ( QEvent* e ) override;
    void resizeEvent(QResizeEvent * event) override;
    void showEvent(QShowEvent * event) override;

private Q_SLOTS:
    void zoom_changed(double zoom);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}

#endif // FONT_FONTPREVIEWWIDGET_H
