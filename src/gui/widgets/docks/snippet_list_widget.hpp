/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SNIPPETLISTWIDGET_H
#define SNIPPETLISTWIDGET_H

#include <memory>
#include <QWidget>

namespace glaxnimate::gui {

class SnippetListWidget : public QWidget
{
    Q_OBJECT

public:
    SnippetListWidget(QWidget* parent = nullptr);
    ~SnippetListWidget();

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void snippet_new();
    void snippet_edit();
    void snippet_delete();
    void snippet_run();
    void snippet_reload();

signals:
    void run_snippet(const QString& source);
    void warning(const QString& message, const QString& title);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // SNIPPETLISTWIDGET_H
