/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
