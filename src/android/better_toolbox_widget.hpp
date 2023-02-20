/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BETTER_STACKED_WIDGET_HPP
#define BETTER_STACKED_WIDGET_HPP

#include <QWidget>
#include <memory>

namespace glaxnimate::android {

/**
 * @brief Drop-in replacement for QToolBox but that doesn't look like utter crap
 */
class BetterToolboxWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BetterToolboxWidget(QWidget *parent = nullptr);
    ~BetterToolboxWidget();

    QWidget* addItem(const QIcon& icon, const QString& text);
    void addItem(QWidget* widget, const QIcon& icon, const QString& text);
    void addItem(QWidget* widget, const QString& text);
    void setItemText(int index, const QString& text);
    int count() const;

    QSize sizeHint() const override;

protected:
    bool event(QEvent *event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};


} // namespace glaxnimate::android
#endif // BETTER_STACKED_WIDGET_HPP
