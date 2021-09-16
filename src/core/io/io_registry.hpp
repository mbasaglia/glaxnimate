#pragma once

#include "base.hpp"
#include "mime/mime_serializer.hpp"

namespace glaxnimate::io {

namespace detail {

inline bool compare_ie_ptr(const ImportExport* ptr_a, const ImportExport* ptr_b) noexcept
{
    return ptr_a->priority() > ptr_b->priority();
}

inline bool compare_ie_unique_ptr(const std::unique_ptr<ImportExport>& ptr_a, const std::unique_ptr<ImportExport>& ptr_b) noexcept
{
    return compare_ie_ptr(ptr_a.get(), ptr_b.get());
}

} // namespace detail

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
        using namespace detail;
        auto iter = std::upper_bound(object_list.begin(), object_list.end(), ie, &compare_ie_unique_ptr);
        ImportExport* format = ie.get();
        object_list.insert(iter, std::move(ie));
        if ( format->can_save() )
            exporters_.insert(std::upper_bound(exporters_.begin(), exporters_.end(), format, &compare_ie_ptr), format);
        if ( format->can_open() )
            importers_.insert(std::upper_bound(importers_.begin(), importers_.end(), format, &compare_ie_ptr), format);
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

    ImportExport* from_extension(const QString& extension, ImportExport::Direction direction) const
    {
        int top_priority = std::numeric_limits<int>::min();
        ImportExport* best = nullptr;
        for ( const auto& p : object_list )
        {
            if ( p->can_handle_extension(extension, direction) && p->priority() > top_priority )
            {
                best = p.get();
                top_priority = p->priority();
            }
        }

        return best;
    }

    ImportExport* from_filename(const QString& filename, ImportExport::Direction direction) const
    {
        int top_priority = std::numeric_limits<int>::min();
        ImportExport* best = nullptr;

        for ( const auto& p : object_list )
        {
            if ( p->can_handle_filename(filename, direction) && p->priority() > top_priority )
            {
                best = p.get();
                top_priority = p->priority();
            }
        }

        return best;
    }

    ImportExport* from_slug(const QString& slug) const
    {
        for ( const auto& p : object_list )
            if ( p->slug() == slug )
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

} // namespace glaxnimate::io
