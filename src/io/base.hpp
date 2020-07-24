#pragma once

#include "app/settings/setting.hpp"
#include "model/document.hpp"

#include <QFileInfo>
#include <QDebug>
#include <QObject>

namespace io {

using Setting = app::settings::Setting;
using SettingList = app::settings::SettingList;

class ImportExport : public QObject
{
    Q_OBJECT

public:
    virtual ~ImportExport() = default;

    bool can_handle_extension(const QString& extension) const
    {
        return extensions().contains(extension);
    }

    bool can_handle_filename(const QString& filename) const
    {
        return can_handle_extension(QFileInfo(filename).completeSuffix());
    }

    /**
     * @pre @p file is open appropriately && @p setting_values contains all the settings correctly && can_open()
     */
    bool open(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        bool ok = on_open(file, filename, document, setting_values);
        emit completed(ok);
        return ok;
    }

    /**
     * @pre @p file is open appropriately && @p setting_values contains all the settings correctly && can_open()
     */
    bool save(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        bool ok = on_save(file, filename, document, setting_values);
        emit completed(ok);
        return ok;
    }

    virtual QString name() const = 0;
    virtual QStringList extensions() const = 0;
    virtual SettingList open_settings() const { return {}; }
    virtual SettingList save_settings() const { return {}; }
    virtual bool can_open() const = 0;
    virtual bool can_save() const = 0;

    /**
     * \brief File dialog name filter
     */
    QString name_filter() const;


    class Factory
    {
    public:
        ImportExport* register_object(std::unique_ptr<ImportExport> ie)
        {
            object_list.push_back(std::move(ie));
            ImportExport* format = object_list.back().get();
            if ( format->can_save() )
                exporters_.push_back(format);
            if ( format->can_open() )
                importers_.push_back(format);
            return format;
        }

        const std::vector<ImportExport*>& importers() const { return importers_; }
        const std::vector<ImportExport*>& exporters() const { return exporters_; }

        const std::vector<std::unique_ptr<ImportExport>>& registered() const
        {
            return object_list;
        }

        ImportExport* from_extension(const QString& extension) const
        {
            for ( const auto& p : object_list )
                if ( p->can_handle_extension(extension) )
                    return p.get();

            return nullptr;
        }

        ImportExport* from_filename(const QString& filename) const
        {
            for ( const auto& p : object_list )
                if ( p->can_handle_filename(filename) )
                    return p.get();

            return nullptr;
        }

    private:
        std::vector<std::unique_ptr<ImportExport>> object_list;
        std::vector<ImportExport*> importers_;
        std::vector<ImportExport*> exporters_;

        Factory() = default;
        ~Factory() = default;
        friend ImportExport;
    };

    static Factory& factory()
    {
        static Factory factory;
        return factory;
    }

protected:
    virtual bool on_open(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        Q_UNUSED(file);
        Q_UNUSED(filename);
        Q_UNUSED(document);
        Q_UNUSED(setting_values);
        return false;
    }

    virtual bool on_save(QIODevice& file, const QString& filename,
                      model::Document* document, const QVariantMap& setting_values)
    {
        Q_UNUSED(file);
        Q_UNUSED(filename);
        Q_UNUSED(document);
        Q_UNUSED(setting_values);
        return false;
    }

signals:
    void error(const QString& message);
    void progress_max_changed(int max);
    void progress(int value);
    void completed(bool success);
};



template<class Derived>
class Autoreg
{
public:
    template<class... Args>
    Autoreg(Args&&... args)
    : registered { static_cast<Derived*>(
        ImportExport::factory().register_object(std::make_unique<Derived>(std::forward<Args>(args)...))
    ) } {}

    Derived* const registered;
};

} // namespace io

