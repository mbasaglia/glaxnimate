/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFile>
#include "app/log/logger.hpp"

namespace app::log {

class ListenerFile: public LogListener
{
public:
    explicit ListenerFile(const QString& filename)
        : file(filename)
    {
        file.open(QFile::WriteOnly|QFile::Text);
    }

protected:
    void on_line(const LogLine& line) override
    {
        QString log;
        log += line.time.toString(Qt::ISODate);
        log += ' '; log += Logger::severity_name(line.severity);
        log += ' '; log += line.source;
        log += ' '; log += line.source_detail;
        log += ' '; log += line.message;
        log += '\n';
        file.write(log.toUtf8());
        file.flush();
    }

private:
    QFile file;
};

} // namespace app::log

