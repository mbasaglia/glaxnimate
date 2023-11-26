/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "export_image_sequence_dialog.hpp"
#include "ui_export_image_sequence_dialog.h"
#include <QEvent>
#include <QImageWriter>
#include <QMessageBox>
#include <QFileDialog>

#include "io/raster/raster_mime.hpp"
#include "math/math.hpp"

class glaxnimate::gui::ExportImageSequenceDialog::Private
{
public:
    Ui::ExportImageSequenceDialog ui;
    model::Composition* comp;
};

glaxnimate::gui::ExportImageSequenceDialog::ExportImageSequenceDialog(
    model::Composition* comp, QDir export_path, QWidget* parent
)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->comp = comp;
    if ( !export_path.exists() )
        export_path = comp->document()->io_options().path;

    d->ui.setupUi(this);
    auto frame_from = comp->animation->first_frame.get();
    auto frame_to = comp->animation->last_frame.get();
    d->ui.input_frame_from->setMinimum(frame_from);
    d->ui.input_frame_from->setMaximum(frame_to);
    d->ui.input_frame_from->setValue(frame_from);
    d->ui.input_frame_to->setMinimum(frame_from);
    d->ui.input_frame_to->setMaximum(frame_to);
    d->ui.input_frame_to->setValue(frame_to);
    d->ui.input_path->setText(export_path.path());

    for ( const auto& ext : QImageWriter::supportedImageFormats() )
    {
        if ( ext != "svg" && ext != "svgz" )
            d->ui.input_format->addItem("." + QString::fromUtf8(ext), QVariant(ext));
    }

    d->ui.input_format->setCurrentText(".png");

    d->ui.progress_bar->hide();
}

glaxnimate::gui::ExportImageSequenceDialog::~ExportImageSequenceDialog() = default;

void glaxnimate::gui::ExportImageSequenceDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

QDir glaxnimate::gui::ExportImageSequenceDialog::export_path() const
{
    return d->ui.input_path->text();
}

void glaxnimate::gui::ExportImageSequenceDialog::render()
{
    auto path = export_path();
    if ( !path.exists() )
    {
        // Why isn't there an easier way?
        auto parent = path;
        parent.cdUp();
        if ( !parent.mkpath(path.dirName()) )
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not create directory\n%1").arg(path.path()));
            return;
        }
    }

    auto name_template = d->ui.input_name->text();
    auto ext = d->ui.input_format->currentText();
    auto format = d->ui.input_format->currentData().toByteArray();

    auto frame_from = d->ui.input_frame_from->value();
    auto frame_to = d->ui.input_frame_to->value();
    auto frame_step = d->ui.input_frame_step->value();


    d->ui.progress_bar->setMinimum(frame_from);
    d->ui.progress_bar->setMaximum(frame_to);
    d->ui.progress_bar->setValue(frame_from);
    d->ui.progress_bar->show();

    auto max = std::max(std::abs(frame_from), std::abs(frame_to));
    auto pad = max != 0 ? std::ceil(std::log(max) / std::log(10)) : 1;
    for ( int f = frame_from; f < frame_to; f += frame_step )
    {
        QString frame_name = QString::number(f).rightJustified(pad, '0');
        QString basename = name_template;
        basename.append(ext);
        basename.replace("{frame}", frame_name);

        QImageWriter(path.filePath(basename), format).write(
            io::raster::RasterMime::frame_to_image(d->comp, f)
        );

        d->ui.progress_bar->setValue(f);
    }

    accept();
}

void glaxnimate::gui::ExportImageSequenceDialog::pick_path()
{
    auto path = QFileDialog::getExistingDirectory(
        this,
        tr("Choose directory"),
        export_path().path(),
        QFileDialog::DontResolveSymlinks
    );
    if ( !path.isEmpty() )
        d->ui.input_path->setText(path);
}
