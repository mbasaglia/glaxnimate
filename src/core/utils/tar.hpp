/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>

#include <QString>
#include <QDir>

#include "app/log/log_line.hpp"

namespace glaxnimate::utils::tar {

class ArchiveEntry
{
public:
    ArchiveEntry(const ArchiveEntry& oth);
    ArchiveEntry(ArchiveEntry&& oth);
    ArchiveEntry& operator=(const ArchiveEntry& oth);
    ArchiveEntry& operator=(ArchiveEntry&& oth);
    bool operator==(const ArchiveEntry& oth) const;
    bool operator!=(const ArchiveEntry& oth) const;
    ~ArchiveEntry();
    const QString& path() const;
    bool valid() const;

private:
    class Private;
    ArchiveEntry(std::unique_ptr<Private> d);
    std::unique_ptr<Private> d;
    friend class TapeArchive;
};

class TapeArchive : public QObject
{
    Q_OBJECT

public:
    class iterator
    {
    public:
        iterator(TapeArchive* archive, ArchiveEntry entry)
            : archive(archive), entry(std::move(entry))
        {}

        const ArchiveEntry* operator->() const { return &entry; }
        const ArchiveEntry& operator*() const { return entry; }
        iterator& operator++()
        {
            entry = archive->next();
            return *this;

        };
        bool operator==(const iterator& oth) const
        {
            return entry == oth.entry && archive == oth.archive;
        }
        bool operator!=(const iterator& oth) const
        {
            return !(*this == oth);
        }

    private:
        TapeArchive* archive;
        ArchiveEntry entry;
    };

    explicit TapeArchive(const QString& filename);
    explicit TapeArchive(const QByteArray& data);
    ~TapeArchive();

    bool extract(const ArchiveEntry& entry, const QDir& destination);

    bool finished() const;
    ArchiveEntry next();
    iterator begin();
    iterator end();

    const QString& error() const;

Q_SIGNALS:
    void message(const QString& message, app::log::Severity Severity);

private:
    class Private;
    std::unique_ptr<Private> d;
    friend class ArchiveEntry;
};


QString libarchive_version();

} // namespace glaxnimate::utils::tar
