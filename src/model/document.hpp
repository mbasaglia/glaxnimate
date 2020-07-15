#pragma once

#include "animation.hpp"

#include <QDir>
#include <QUndoStack>


namespace model {

class Document : public QObject
{
    Q_OBJECT

public:
    Document();
    ~Document();

    QString source_filename() const;
    void set_source_filename(const QString& n);

    QDir save_path() const;
    void set_save_path(const QDir& p);

    QVariantMap& metadata() const;

    Animation& animation();

    QUndoStack& undo_stack();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
