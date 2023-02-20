/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "format_selection_dialog.hpp"

#include <QPushButton>
#include <QVBoxLayout>

#include <QDebug>

class glaxnimate::android::FormatSelectionDialog::Private
{
public:
    io::ImportExport* format = nullptr;
};

glaxnimate::android::FormatSelectionDialog::FormatSelectionDialog(QWidget *parent)
    : BaseDialog(parent),
      d(std::make_unique<Private>())
{
    QVBoxLayout* lay = new QVBoxLayout(this);
    setLayout(lay);

    for ( const auto& fmt : io::IoRegistry::instance().registered() )
    {
        if ( fmt->slug() == "raster" )
            continue;
        QPushButton *btn = new QPushButton(this);
        lay->addWidget(btn);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setText(fmt->name());
        connect(btn, &QPushButton::clicked, this, [this, fmt=fmt.get()](){
            d->format = fmt;
            accept();
        });
    }
}

glaxnimate::android::FormatSelectionDialog::~FormatSelectionDialog()
{}

glaxnimate::io::ImportExport *glaxnimate::android::FormatSelectionDialog::format() const
{
    return d->format;
}
