#pragma once


#include <QDir>
#include <QUndoStack>

#include "animation.hpp"
#include "io/saved_io_options.hpp"

namespace model {

namespace graphics {
    class DocumentScene;
} // namespace graphics

class Document : public QObject
{
    Q_OBJECT

public:
    explicit Document(const QString& filename);
    ~Document();

    QString filename() const;

    QVariantMap& metadata() const;

    Animation& animation();

    QUndoStack& undo_stack();

    const io::SavedIoOptions& export_options() const;

    void set_export_options(const io::SavedIoOptions& opt);

    graphics::DocumentScene& graphics_scene() const;

signals:
    void filename_changed(const QString& n);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
