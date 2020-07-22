#pragma once

#include <QFileDialog>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "io/exporter.hpp"
#include "model/document.hpp"
#include "app/settings/widget_builder.hpp"


class ImportExportDialog
{

public:
    ImportExportDialog(QWidget* parent)
        : parent(parent)
    {

    }

    const io::Options& io_options() const
    {
        return io_options_;
    }

    bool export_dialog(model::Document* doc)
    {
        io_options_ = doc->export_options();
        QFileDialog dialog(parent);
        dialog.setWindowTitle(QObject::tr("Save file"));
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        setup_file_dialog(dialog, io::Exporter::factory(), io_options_.method, false);
        while ( true )
        {
            if ( !show_file_dialog(dialog) )
                return false;

            // The file dialog already asks whether to overwrite, but not if we appended the extension ourselves
            if ( !name_changed )
                return true;

            QFileInfo finfo(io_options_.filename);
            if ( finfo.exists() )
            {
                QMessageBox overwrite(
                    QMessageBox::Question,
                    QObject::tr("Overwrite File?"),
                    QObject::tr("The file \"%1\" already exists. Do you wish to overwrite it?")
                        .arg(finfo.baseName()),
                    QMessageBox::Yes|QMessageBox::No,
                    parent
                );
                overwrite.setDefaultButton(QMessageBox::Yes);
                if ( overwrite.exec() == QMessageBox::Yes )
                    return true;
            }
            else
            {
                return true;
            }
        }
    }

    bool options_dialog()
    {
        if ( !io_options_.method )
            return false;

        if ( io_options_.method->settings().empty() )
            return true;

        QDialog dialog(parent);

        dialog.setWindowTitle(QObject::tr("%1 Options").arg(io_options_.method->name()));

        QFormLayout layout;
        dialog.setLayout(&layout);

        app::settings::WidgetBuilder widget_builder;
        widget_builder.add_widgets(io_options_.method->settings(), &dialog, &layout, io_options_.settings);
        QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout.setWidget(1, QFormLayout::SpanningRole, &box);
        QObject::connect(&box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(&box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if ( dialog.exec() == QDialog::Rejected )
            return false;

        return true;
    }

private:
    bool show_file_dialog(QFileDialog& dialog)
    {
        io_options_.method = nullptr;

        if ( dialog.exec() == QDialog::Rejected )
            return false;

        io_options_.filename = dialog.selectedFiles()[0];
        io_options_.path = dialog.directory();

        name_changed = false;
        int filter = filters.indexOf(dialog.selectedNameFilter());
        if ( filter < int(io::Exporter::factory().registered().size()) )
        {
            io_options_.method = io::Exporter::factory().registered()[filter].get();
            // For some reason the file dialog shows the option to do this automatically but it's disabled
            if ( QFileInfo(io_options_.filename).completeSuffix().isEmpty() )
            {
                io_options_.filename += "." + io_options_.method->extensions()[0];
                name_changed = true;
            }
        }
        else
        {
            io_options_.method = io::Exporter::factory().from_filename(io_options_.filename);
        }

        if ( !io_options_.method )
            return false;

        return true;
    }

    template<class T>
    void setup_file_dialog(QFileDialog& dialog, const io::ImportExportFactory<T>& fac, io::ImportExport* selected, bool include_all)
    {
        dialog.setDirectory(io_options_.path);
        dialog.selectFile(io_options_.filename);

        filters.clear();

        QString all;
        for ( const auto& reg : fac.registered() )
        {
            for ( const QString& ext : reg->extensions() )
            {
                all += ext + " ";
            }

            filters << reg->name_filter();
        }

        if ( include_all )
        {
            all.resize(all.size() - 1);
            filters << QObject::tr("All files (%1)").arg(all);
        }

        dialog.setNameFilters(filters);

        if ( selected )
            dialog.selectNameFilter(selected->name_filter());
    }

    QWidget* parent;
    QStringList filters;
    io::Options io_options_;
    bool name_changed = false;
};

