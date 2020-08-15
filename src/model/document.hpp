#pragma once


#include <QDir>
#include <QUndoStack>

#include "animation.hpp"
#include "io/options.hpp"

namespace model {

class Document : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename)
    Q_PROPERTY(Animation* animation READ animation)
    Q_PROPERTY(double current_time READ current_time WRITE set_current_time NOTIFY current_time_changed)

public:
    explicit Document(const QString& filename);
    ~Document();

    QString filename() const;

    QVariantMap& metadata() const;

    Animation* animation();

    QUndoStack& undo_stack();

    const io::Options& io_options() const;

    void set_io_options(const io::Options& opt);

    DocumentNode* node_by_uuid(const QUuid& n) const;

    Q_INVOKABLE bool undo();
    Q_INVOKABLE bool redo();

    FrameTime current_time() const;
    void set_current_time(FrameTime t);

    QSize size() const;

    /**
     * \brief Whether filename() refers to an actual file
     */
    bool has_file() const;

    void set_has_file(bool hf);

signals:
    void filename_changed(const QString& n);
    void current_time_changed(FrameTime t);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
