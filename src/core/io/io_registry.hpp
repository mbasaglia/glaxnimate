#pragma once

#include "base.hpp"
#include "mime/mime_serializer.hpp"

namespace io {

class IoRegistry
{
public:
    static IoRegistry& instance()
    {
        static IoRegistry factory;
        return factory;
    }

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

    void unregister(ImportExport* object)
    {
        for ( auto it = object_list.begin(); it != object_list.end(); ++it )
        {
            if ( it->get() == object )
            {
                object_list.erase(it);
                break;
            }
        }
        importers_.erase(std::remove(importers_.begin(), importers_.end(), object), importers_.end());
        exporters_.erase(std::remove(exporters_.begin(), exporters_.end(), object), exporters_.end());
    }

    mime::MimeSerializer* register_object(std::unique_ptr<mime::MimeSerializer> ie)
    {
        mime_list.push_back(std::move(ie));
        mime::MimeSerializer* format = mime_list.back().get();
        mime_pointers.push_back(format);
        return format;
    }

    const std::vector<ImportExport*>& importers() const { return importers_; }
    const std::vector<ImportExport*>& exporters() const { return exporters_; }
    const std::vector<mime::MimeSerializer*>& serializers() const { return mime_pointers; }

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

    mime::MimeSerializer* serializer_from_slug(const QString& slug) const
    {
        for ( const auto& serializer : mime_list )
        {
            if ( serializer->slug() == slug )
                return serializer.get();
        }

        return nullptr;
    }

private:
    std::vector<std::unique_ptr<ImportExport>> object_list;
    std::vector<ImportExport*> importers_;
    std::vector<ImportExport*> exporters_;
    std::vector<std::unique_ptr<mime::MimeSerializer>> mime_list;
    std::vector<mime::MimeSerializer*> mime_pointers;

    IoRegistry() = default;
    ~IoRegistry() = default;
};


template<class Derived>
class Autoreg
{
public:
    template<class... Args>
    Autoreg(Args&&... args)
    : registered { static_cast<Derived*>(
        IoRegistry::instance().register_object(std::make_unique<Derived>(std::forward<Args>(args)...))
    ) } {}

    Derived* const registered;
};

} // namespace io
