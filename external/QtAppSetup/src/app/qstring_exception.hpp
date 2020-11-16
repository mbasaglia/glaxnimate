#pragma once

#include <stdexcept>
#include <QString>

namespace app {

template<class Base=std::runtime_error>
class QStringException : public Base
{
protected:
    using Ctor = QStringException;

public:
    QStringException(const QString& what) : Base(what.toStdString()) {}

    QString message() const
    {
        return QString(this->what());
    }
};

} // namespace app
