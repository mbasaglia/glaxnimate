#include "tar.hpp"

#include <archive.h>
#include <archive_entry.h>

#include "app/log/log.hpp"

class glaxnimate::utils::tar::ArchiveEntry::Private
{
public:
    Private(archive_entry *entry)
        : entry(entry),
#if ARCHIVE_VERSION_NUMBER > 3001002
          path(QString::fromUtf8(archive_entry_pathname_utf8(entry)))
#else
          path(archive_entry_pathname(entry))
#endif
    {}

    archive_entry *entry;
    QString path;
};

glaxnimate::utils::tar::ArchiveEntry::ArchiveEntry(std::unique_ptr<Private> d)
    : d(std::move(d))
{}

glaxnimate::utils::tar::ArchiveEntry::ArchiveEntry(const glaxnimate::utils::tar::ArchiveEntry& oth)
    : d(std::make_unique<Private>(*oth.d))
{
}

glaxnimate::utils::tar::ArchiveEntry & glaxnimate::utils::tar::ArchiveEntry::operator=(const ArchiveEntry& oth)
{
    *d = *oth.d;
    return *this;
}

glaxnimate::utils::tar::ArchiveEntry::ArchiveEntry(glaxnimate::utils::tar::ArchiveEntry && oth) = default;
glaxnimate::utils::tar::ArchiveEntry & glaxnimate::utils::tar::ArchiveEntry::operator=(ArchiveEntry && oth)  = default;

glaxnimate::utils::tar::ArchiveEntry::~ArchiveEntry() = default;

bool glaxnimate::utils::tar::ArchiveEntry::operator==(const glaxnimate::utils::tar::ArchiveEntry& oth) const
{
    if ( !d != !oth.d )
        return false;
    if ( !d )
        return true;
    return d->entry == oth.d->entry;
}

bool glaxnimate::utils::tar::ArchiveEntry::operator!=(const glaxnimate::utils::tar::ArchiveEntry& oth) const
{
    return !(*this == oth);
}

const QString & glaxnimate::utils::tar::ArchiveEntry::path() const
{
    return d->path;
}

bool glaxnimate::utils::tar::ArchiveEntry::valid() const
{
    return d && d->entry;
}


class glaxnimate::utils::tar::TapeArchive::Private
{
public:
    Private(TapeArchive* parent) : parent(parent) {}

    void open(const QString& filename)
    {
        input = archive_read_new();
        archive_read_support_format_all(input);
        archive_read_support_filter_all(input);
        int result = archive_read_open_filename(input, filename.toStdString().c_str(), 10240);
        if ( result < 0 )
        {
            handle_message(result, input);
            close();
        }
        else
        {
            finished = false;
        }
    }

    void load_data(const QByteArray& data)
    {
        input = archive_read_new();
        archive_read_support_format_all(input);
        archive_read_support_filter_all(input);
        int result = archive_read_open_memory(input, (void*)data.data(), data.size());
        if ( result < 0 )
        {
            handle_message(result, input);
            close();
        }
        else
        {
            finished = false;
        }
    }

    int copy_data(archive *output)
    {
        int result;
        const void *buff;
        size_t size;
        int64_t offset;

        while ( true )
        {
            result = archive_read_data_block(input, &buff, &size, &offset);

            if ( result == ARCHIVE_EOF )
                return ARCHIVE_OK;

            if ( result < ARCHIVE_OK )
            {
                handle_message(result, input);
                return result;
            }

            result = archive_write_data_block(output, buff, size, offset);
            if ( result < ARCHIVE_OK )
            {
                handle_message(result, output);
                return result;
            }
        }
    }

    void extract_begin()
    {
        output = archive_write_disk_new();
        archive_write_disk_set_options(output, ARCHIVE_EXTRACT_TIME|ARCHIVE_EXTRACT_PERM|ARCHIVE_EXTRACT_ACL|ARCHIVE_EXTRACT_FFLAGS);
        archive_write_disk_set_standard_lookup(output);
    }

    archive_entry* next_entry()
    {
        if ( !input || !output || finished )
            return nullptr;

        while ( true )
        {
            archive_entry *entry;
            int result = archive_read_next_header(input, &entry);
            if ( result == ARCHIVE_EOF )
            {
                finished = true;
                return nullptr;
            }

            if ( result < ARCHIVE_OK )
                handle_message(result, input);
            if ( result == ARCHIVE_FAILED )
                continue;
            if ( result == ARCHIVE_FATAL )
            {
                finished = true;
                return nullptr;
            }

            if ( archive_entry_size(entry) < 0 )
                continue;

            return entry;
        }
    }

    bool extract(const glaxnimate::utils::tar::ArchiveEntry& entry, const QDir& destination)
    {
        QString output_file_path = destination.absoluteFilePath(entry.d->path);

#if ARCHIVE_VERSION_NUMBER > 3001002
        archive_entry_set_pathname_utf8(entry.d->entry, output_file_path.toStdString().c_str());
#else
        archive_entry_set_pathname(entry.d->entry, output_file_path.toStdString().c_str());
#endif

        int result = archive_write_header(output, entry.d->entry);
        if ( result < ARCHIVE_OK )
        {
            handle_message(result, output);
        }
        else
        {
            result = copy_data(output);
            if ( result == ARCHIVE_FAILED )
                return false;
            if ( result == ARCHIVE_FATAL )
            {
                finished = true;
                return false;
            }
        }

        result = archive_write_finish_entry(output);
        if ( result < ARCHIVE_OK )
            handle_message(result, output);

        if ( result == ARCHIVE_FATAL )
            finished = true;

        return result >= ARCHIVE_WARN;
    }

    void handle_message(int result, archive* arch)
    {
        if ( result < ARCHIVE_OK )
        {
            QString message = archive_error_string(arch);

            app::log::Severity severity = app::log::Info;
            if ( result == ARCHIVE_FATAL )
            {
                error = message;
                severity = app::log::Error;
            }
            else if ( result < ARCHIVE_WARN )
            {
                severity = app::log::Warning;
            }

            app::log::Log("tar").log(message, severity);
            parent->message(message, severity);
        }
    }

    void extract_end()
    {
        if ( output )
        {
            archive_write_close(output);
            archive_write_free(output);
            output = nullptr;
        }
    }

    void close()
    {
        extract_end();

        if ( input )
        {
            archive_read_close(input);
            archive_read_free(input);
            input = nullptr;
        }
    }

    archive* input = nullptr;
    archive* output = nullptr;
    TapeArchive* parent;
    QString error;
    bool finished = true;
};


glaxnimate::utils::tar::TapeArchive::TapeArchive(const QString& filename)
    : d(std::make_unique<Private>(this))
{
    d->open(filename);
}

glaxnimate::utils::tar::TapeArchive::TapeArchive(const QByteArray& data)
    : d(std::make_unique<Private>(this))
{
    d->load_data(data);
}

glaxnimate::utils::tar::TapeArchive::~TapeArchive()
{
    d->close();
}

const QString & glaxnimate::utils::tar::TapeArchive::error() const
{
    return d->error;
}

bool glaxnimate::utils::tar::TapeArchive::finished() const
{
    return d->finished;
}

glaxnimate::utils::tar::ArchiveEntry glaxnimate::utils::tar::TapeArchive::next()
{
    if ( d->finished )
        return ArchiveEntry({});

    if ( !d->output )
        d->extract_begin();

    if ( auto entry = d->next_entry() )
        return ArchiveEntry(std::make_unique<ArchiveEntry::Private>(entry));

    d->extract_end();
    return ArchiveEntry({});
}


bool glaxnimate::utils::tar::TapeArchive::extract(const glaxnimate::utils::tar::ArchiveEntry& entry, const QDir& destination)
{
    return d->extract(entry, destination);
}

glaxnimate::utils::tar::TapeArchive::iterator glaxnimate::utils::tar::TapeArchive::begin()
{
    return iterator(this, next());
}

glaxnimate::utils::tar::TapeArchive::iterator glaxnimate::utils::tar::TapeArchive::end()
{
    return iterator(this, ArchiveEntry({}));
}

QString glaxnimate::utils::tar::libarchive_version()
{
    int vint = ARCHIVE_VERSION_NUMBER;
    int patch = vint % 1000;
    vint /= 1000;
    int minor = vint % 1000;
    vint /= 1000;

    return QString("%1.%2.%3").arg(vint).arg(minor).arg(patch);
}
