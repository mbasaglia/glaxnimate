#pragma once

#include "animation.hpp"

#include <QDir>
#include <QUndoStack>


namespace model {

class Document : public QObject
{
    Q_OBJECT

public:
    explicit Document(const QString& filename);
    ~Document();

    QString filename() const;

    QDir save_path() const;
    void set_save_path(const QDir& p);

    QVariantMap& metadata() const;

    Animation& animation();

    QUndoStack& undo_stack();

public slots:
    void set_filename(const QString& n);

signals:
    void filename_changed(const QString& n);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
