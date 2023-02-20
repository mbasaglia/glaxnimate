/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDebug>
#include "app/log/logger.hpp"

namespace app::log {

class ListenerStderr: public LogListener
{
protected:
    void on_line(const LogLine& line) override
    {
        QMessageLogger logger;
        QDebug stream = logger.warning();
        if ( line.severity == Info )
            stream = logger.info();

        stream
            << line.time.toString(Qt::ISODate)
            << Logger::severity_name(line.severity)
            << line.source << line.source_detail << line.message
        ;
    }
};

} // namespace app::log
