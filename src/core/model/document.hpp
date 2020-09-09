#pragma once


#include <QDir>
#include <QUndoStack>

#include "main_composition.hpp"
#include "io/options.hpp"

namespace model {

class Document : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename)
    Q_PROPERTY(MainComposition* main_composition READ main_composition)
    Q_PROPERTY(double current_time READ current_time WRITE set_current_time NOTIFY current_time_changed)
    Q_PROPERTY(bool record_to_keyframe READ record_to_keyframe WRITE set_record_to_keyframe NOTIFY record_to_keyframe_changed)

public:
    explicit Document(const QString& filename);
    ~Document();

    QString filename() const;

    QVariantMap& metadata() const;

    MainComposition* main_composition();

    QUndoStack& undo_stack();

    const io::Options& io_options() const;

    void set_io_options(const io::Options& opt);

    Q_INVOKABLE model::DocumentNode* find_by_uuid(const QUuid& n) const;
    Q_INVOKABLE model::DocumentNode* find_by_name(const QString& name) const;
    Q_INVOKABLE QVariantList find_by_type_name(const QString& type_name) const;

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

    /**
     * \brief Whether animated values should add keyframes when their value changes
     */
    bool record_to_keyframe() const;
    void set_record_to_keyframe(bool r);

signals:
    void filename_changed(const QString& n);
    void current_time_changed(FrameTime t);
    void record_to_keyframe_changed(bool r);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
