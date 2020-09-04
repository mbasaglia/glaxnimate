#pragma once

#include <QFileDialog>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "io/base.hpp"
#include "model/document.hpp"
#include "app/settings/widget_builder.hpp"


class ImportExportDialog
{

public:
    ImportExportDialog(const io::Options& options, QWidget* parent)
        : parent(parent)
    {
        io_options_ = options;

    }

    const io::Options& io_options() const
    {
        return io_options_;
    }

    bool export_dialog()
    {
        QFileDialog dialog(parent);
        dialog.setWindowTitle(QObject::tr("Save file"));
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setFileMode(QFileDialog::AnyFile);
        setup_file_dialog(dialog, io::ImportExport::factory().exporters(), io_options_.format, false);
        while ( true )
        {
            if ( !show_file_dialog(dialog, io::ImportExport::factory().exporters()) )
                return false;

            // For some reason the file dialog shows the option to do this automatically but it's disabled
            if ( QFileInfo(io_options_.filename).completeSuffix().isEmpty() )
            {
                io_options_.filename += "." + io_options_.format->extensions()[0];

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
                    if ( overwrite.exec() != QMessageBox::Yes )
                        continue;
                }
            }

            break;
        }

        return options_dialog(io_options_.format->save_settings());
    }

    bool import_dialog()
    {
        QFileDialog dialog(parent);
        dialog.setWindowTitle(QObject::tr("Open file"));
        dialog.setAcceptMode(QFileDialog::AcceptOpen);
        dialog.setFileMode(QFileDialog::ExistingFile);
        setup_file_dialog(dialog, io::ImportExport::factory().importers(), io_options_.format, true);
        if ( show_file_dialog(dialog, io::ImportExport::factory().importers()) )
            return options_dialog(io_options_.format->open_settings());
        return false;
    }

    bool options_dialog(const io::SettingList& settings)
    {
        if ( !io_options_.format )
            return false;

        if ( settings.empty() )
            return true;

        app::settings::WidgetBuilder widget_builder;
        QString title = QObject::tr("%1 Options").arg(io_options_.format->name());
        return widget_builder.show_dialog(settings, io_options_.settings, title, parent);
    }
private:
    bool show_file_dialog(QFileDialog& dialog, const std::vector<io::ImportExport*>& formats)
    {
        io_options_.format = nullptr;

        if ( dialog.exec() == QDialog::Rejected )
            return false;

        io_options_.filename = dialog.selectedFiles()[0];
        io_options_.path = dialog.directory();

        int filter = filters.indexOf(dialog.selectedNameFilter());
        if ( filter < int(formats.size()) )
        {
            io_options_.format = formats[filter];
        }
        else
        {
            io_options_.format = io::ImportExport::factory().from_filename(io_options_.filename);
        }

        if ( !io_options_.format )
            return false;

        return true;
    }

    void setup_file_dialog(QFileDialog& dialog, const std::vector<io::ImportExport*>& formats,
                           io::ImportExport* selected, bool add_all)
    {
        dialog.setDirectory(io_options_.path);
        dialog.selectFile(io_options_.filename);

        filters.clear();

        QString all;
        for ( const auto& reg : formats )
        {
            for ( const QString& ext : reg->extensions() )
            {
                all += ext + " ";
            }

            filters << reg->name_filter();
        }

        if ( add_all )
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
};

