/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <QObject>

#include "app/log/log_line.hpp"

namespace app::log {

class Logger;

class LogListener
{
public:
    LogListener() {}
    virtual ~LogListener() = default;

protected:
    virtual void on_line(const LogLine& line) = 0;

    friend class Logger;
};

class Logger : public QObject
{
    Q_OBJECT

public:
    static QString severity_name(Severity s)
    {
        switch ( s )
        {
            case Info:
                return "Info";
            case Warning:
                return "Warning";
            case Error:
                return "Error";
            default:
                return "?";
        }
    }

    static Logger& instance()
    {
        static Logger instance;
        return instance;
    }

    template<class T, class... Args>
    T* add_listener(Args&&... args)
    {
        listeners.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return static_cast<T*>(listeners.back().get());
    }

public Q_SLOTS:
    void log(const LogLine& line)
    {
        for ( const auto& listener : listeners )
            listener->on_line(line);
        emit logged(line);
    }

signals:
    void logged(const LogLine& line);

private:
    Logger() = default;
    ~Logger() = default;
    std::vector<std::unique_ptr<LogListener>> listeners;
};

} // namespace app::log
