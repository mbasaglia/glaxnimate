/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QTextStream>

#include "app/log/logger.hpp"

namespace app::log {

class LogStream
{
public:
    LogStream(const QString& source, const QString& detail = "", Severity severity = Warning)
        : source(source), detail(detail), severity(severity)
    {}

    ~LogStream()
    {
        if ( !message.isEmpty() )
            Logger::instance().log({severity, source, detail, message, QDateTime::currentDateTime()});
    }

    template<class T>
    LogStream& operator<<(T&& item)
    {
        if ( !message.isEmpty() )
            str << ' ';
        str << std::forward<T>(item);
        return *this;
    }

private:
    QString source;
    QString detail;
    Severity severity;
    QString message;
    QTextStream str{&message};
};

class Log
{
public:
    Log(const QString& source, const QString& detail="")
    : source(source), detail(detail)
    {}

    const Log& log(const QString& message, Severity severity = Warning) const
    {
        Logger::instance().log({severity, source, detail, message, QDateTime::currentDateTime()});
        return *this;
    }

    void operator()(const QString& message, Severity severity = Warning)
    {
        log(message, severity);
    }

    void set_detail(const QString& detail)
    {
        this->detail = detail;
    }

    LogStream stream(Severity severity = Warning) const
    {
        return LogStream(source, detail, severity);
    }

private:
    QString source;
    QString detail;
};

} // namespace app::log
