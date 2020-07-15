#pragma once

#include "animation.hpp"

#include <QDir>


namespace model {

class Document : public QObject
{
    Q_OBJECT

public:

    QString source_filename() const;
    void set_source_filename(const QString& n);

    QDir save_path() const;
    void set_save_path(const QDir& p);

    QVariantMap& metadata() const;

    Animation& animation();
};

} // namespace model
