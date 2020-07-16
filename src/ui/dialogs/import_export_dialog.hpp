#pragma once

#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDialogButtonBox>

#include "io/exporter.hpp"
#include "model/document.hpp"


class ImportExportDialog
{

public:
    ImportExportDialog(QWidget* parent)
        : parent(parent)
    {

    }

    const io::SavedIoOptions& io_options() const
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
        setup_file_dialog(dialog);
        setup_filters(dialog, io::Exporter::factory(), false);
        return show_file_dialog(dialog);
    }

    bool options_dialog()
    {
        if ( !io_options_.method )
            return false;

        QDialog dialog(parent);

        dialog.setWindowTitle(QObject::tr("%s Options").arg(io_options_.method->name()));

        QFormLayout layout;
        dialog.setLayout(&layout);

        for ( const io::Option& opt : io_options_.method->options() )
        {
            io_options_.options[opt.slug] = opt.get_variant(io_options_.options);
            QWidget* wid = make_option_widget(opt);
            if ( !wid )
                continue;

            QLabel* label = new QLabel(opt.name, &dialog);
            label->setToolTip(opt.description);
            wid->setParent(&dialog);
            wid->setToolTip(opt.description);
            wid->setWhatsThis(opt.description);
            layout.addRow(label, wid);
        }

        QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout.setWidget(1, QFormLayout::SpanningRole, &box);
        QObject::connect(&box, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(&box, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if ( dialog.exec() == QDialog::Rejected )
            return false;

        return true;
    }

private:
    QWidget* make_option_widget(const io::Option& opt)
    {
        if ( opt.type == io::Option::Info )
        {
            return new QLabel(opt.description);
        }
        else if ( opt.type == io::Option::Bool )
        {
            auto wid = new QCheckBox();
            wid->setChecked(opt.get<bool>(io_options_.options));
            QObject::connect(wid, &QCheckBox::toggled, OptionSetter<bool>{opt.slug, &io_options_.options});
            return wid;
        }
        else if ( opt.type == io::Option::Int )
        {
            auto wid = new QSpinBox();
            wid->setValue(opt.get<int>(io_options_.options));
            wid->setMinimum(opt.min);
            wid->setMaximum(opt.max);
            QObject::connect(wid, (void(QSpinBox::*)(int))&QSpinBox::valueChanged,
                             OptionSetter<int>{opt.slug, &io_options_.options});
            return wid;
        }
        else if ( opt.type == io::Option::Float )
        {
            auto wid = new QDoubleSpinBox();
            wid->setValue(opt.get<float>(io_options_.options));
            wid->setMinimum(opt.min);
            wid->setMaximum(opt.max);
            QObject::connect(wid, (void(QDoubleSpinBox::*)(double))&QDoubleSpinBox::valueChanged,
                             OptionSetter<float>{opt.slug, &io_options_.options});
            return wid;
        }
        else if ( opt.type == io::Option::String )
        {
            auto wid = new QLineEdit();
            wid->setText(opt.get<QString>(io_options_.options));
            QObject::connect(wid, &QLineEdit::textChanged, OptionSetter<QString>{opt.slug, &io_options_.options});
            return wid;
        }

        return nullptr;
    }

    bool show_file_dialog(QFileDialog& dialog)
    {
        io_options_.method = nullptr;

        if ( dialog.exec() == QDialog::Rejected )
            return false;

        io_options_.filename = dialog.selectedFiles()[0];
        io_options_.path = dialog.directory();

        int filter = filters.indexOf(dialog.selectedNameFilter());
        if ( filter < int(io::Exporter::factory().registered().size()) )
            io_options_.method = io::Exporter::factory().registered()[filter].get();
        else
            io_options_.method = io::Exporter::factory().from_filename(io_options_.filename);

        if ( !io_options_.method )
            return false;

        return true;
    }

    void setup_file_dialog(QFileDialog &dialog)
    {
        QString suf = QFileInfo(io_options_.filename).completeSuffix();
        if ( suf.isEmpty() )
            suf = "json";
        dialog.setDefaultSuffix(suf);

        dialog.setDirectory(io_options_.path);
        dialog.selectFile(io_options_.filename);
    }

    template<class T>
    void setup_filters(QFileDialog& dialog, const io::ImportExportFactory<T>& fac, bool include_all)
    {
        filters.clear();

        QString all;
        for ( const auto& reg : fac.registered() )
        {
            QString ext_str;
            for ( const QString& ext : reg->extensions() )
            {
                ext_str += ext + " ";
            }

            if ( ext_str.isEmpty() )
                continue;

            all += ext_str;
            ext_str.resize(ext_str.size() - 1);
            filters << QObject::tr("%1 (%2)").arg(reg->name()).arg(ext_str);
        }

        if ( include_all )
        {
            all.resize(all.size() - 1);
            filters << QObject::tr("All files (%1)").arg(all);
        }

        dialog.setNameFilters(filters);
    }

    template<class T>
    struct OptionSetter
    {
        QString name;
        QVariantMap* options;

        void operator()(T v)
        {
            (*options)[name] = QVariant(v);
        }
    };

    QWidget* parent;
    QStringList filters;
    io::SavedIoOptions io_options_;
};

