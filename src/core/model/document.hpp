#pragma once


#include <QDir>
#include <QUndoStack>

#include "main_composition.hpp"
#include "io/options.hpp"
// #include "defs/defs.hpp"

namespace model {

class Defs;

class Document : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename)
    Q_PROPERTY(model::MainComposition* main READ main)
    Q_PROPERTY(double current_time READ current_time WRITE set_current_time NOTIFY current_time_changed)
    Q_PROPERTY(bool record_to_keyframe READ record_to_keyframe WRITE set_record_to_keyframe NOTIFY record_to_keyframe_changed)
    Q_PROPERTY(Object* defs READ defs_obj)

public:
    explicit Document(const QString& filename);
    ~Document();

    QString filename() const;

    QVariantMap& metadata() const;

    MainComposition* main();

    QUndoStack& undo_stack();

    const io::Options& io_options() const;

    void set_io_options(const io::Options& opt);

    Q_INVOKABLE model::ReferenceTarget* find_by_uuid(const QUuid& n) const;
    Q_INVOKABLE model::DocumentNode* find_by_name(const QString& name) const;
    Q_INVOKABLE QVariantList find_by_type_name(const QString& type_name) const;

    Q_INVOKABLE bool undo();
    Q_INVOKABLE bool redo();

    void push_command(QUndoCommand* cmd);

    FrameTime current_time() const;
    void set_current_time(FrameTime t);

    Q_INVOKABLE QSize size() const;
    Q_INVOKABLE QRectF rect() const;

    /**
     * \brief Whether animated values should add keyframes when their value changes
     */
    bool record_to_keyframe() const;
    void set_record_to_keyframe(bool r);

    Q_INVOKABLE QString get_best_name(const model::DocumentNode* node, const QString& suggestion={}) const;
    Q_INVOKABLE void set_best_name(model::DocumentNode* node, const QString& suggestion={}) const;

    Q_INVOKABLE QImage render_image(FrameTime time, QSize size) const;
    Q_INVOKABLE QImage render_image() const;

    model::Defs* defs() const;

signals:
    void filename_changed(const QString& n);
    void current_time_changed(FrameTime t);
    void record_to_keyframe_changed(bool r);

private:
    Object* defs_obj() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
