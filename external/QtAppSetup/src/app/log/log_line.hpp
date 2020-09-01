#pragma once

#include <QString>
#include <QDateTime>
#include <QMetaType>

namespace app::log {

enum Severity
{
    Info,
    Warning,
    Error,
};

struct LogLine
{
    Severity severity;
    QString source;
    QString source_detail;
    QString message;
    QDateTime time;
};


} // namespace app::log

Q_DECLARE_METATYPE(app::log::LogLine)
