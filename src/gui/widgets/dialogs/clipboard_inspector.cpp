/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "clipboard_inspector.hpp"
#include <QTabWidget>
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QTextBlock>
#include <QPushButton>


using namespace glaxnimate::gui;

ClipboardInspector::ClipboardInspector(QWidget* parent)
    : QDialog(parent)
{
    resize(1024, 1024);

    auto lay = new QVBoxLayout(this);
    setLayout(lay);

    auto tab = new QTabWidget(this);
    lay->addWidget(tab);
    load(tab);

    QPushButton* button = new QPushButton(this);
    button->setText("Reload");
    lay->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, tab]{
        while ( tab->count() )
            delete tab->widget(0);
        load(tab);
    });
}

void ClipboardInspector::load(QTabWidget* tab)
{
    auto data = QGuiApplication::clipboard()->mimeData();

    for ( const auto& format : data->formats() )
    {
        QPlainTextEdit* widget = new QPlainTextEdit(this);
        auto bytes = data->data(format);
        widget->appendPlainText(bytes);
        if ( !format.contains("json") && !format.contains("text") && !format.contains("xml") )
        {
            widget->appendPlainText("\n\n\n");
            widget->appendPlainText(bytes.toHex());
        }
        widget->setTextCursor(QTextCursor(widget->document()->findBlockByLineNumber(0)));
        tab->addTab(widget, format);
    }
}
