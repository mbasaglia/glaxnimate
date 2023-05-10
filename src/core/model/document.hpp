/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include <QDir>
#include <QUndoStack>

#include "io/options.hpp"
#include "model/comp_graph.hpp"
#include "model/document_node.hpp"

namespace glaxnimate::model {

class Assets;
struct PendingAsset;
class Composition;

class Document : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ filename)
    Q_PROPERTY(double current_time READ current_time WRITE set_current_time NOTIFY current_time_changed)
    Q_PROPERTY(bool record_to_keyframe READ record_to_keyframe WRITE set_record_to_keyframe NOTIFY record_to_keyframe_changed)
    Q_PROPERTY(Object* assets READ assets_obj)
    Q_PROPERTY(QVariantMap metadata READ metadata WRITE set_metadata)

public:
    struct DocumentInfo
    {
        QString author;
        QString description;
        QStringList keywords;

        bool empty() const
        {
            return author.isEmpty() && description.isEmpty() && keywords.empty();
        }
    };

    explicit Document(const QString& filename = {});
    ~Document();

    QString filename() const;
    QUuid uuid() const;

    QVariantMap& metadata();
    void set_metadata(const QVariantMap& meta);

    DocumentInfo& info();

    Composition* current_comp();
    void set_current_comp(Composition* comp);

    QUndoStack& undo_stack();

    const io::Options& io_options() const;

    void set_io_options(const io::Options& opt);

    Q_INVOKABLE glaxnimate::model::DocumentNode* find_by_uuid(const QUuid& n) const;
    Q_INVOKABLE glaxnimate::model::DocumentNode* find_by_name(const QString& name) const;
    Q_INVOKABLE QVariantList find_by_type_name(const QString& type_name) const;

    Q_INVOKABLE bool undo();
    Q_INVOKABLE bool redo();

    void push_command(QUndoCommand* cmd);

    FrameTime current_time() const;
    void set_current_time(FrameTime t);

    /**
     * \brief Whether animated values should add keyframes when their value changes
     */
    bool record_to_keyframe() const;
    void set_record_to_keyframe(bool r);

    Q_INVOKABLE QString get_best_name(glaxnimate::model::DocumentNode* node, const QString& suggestion={}) const;
    Q_INVOKABLE void set_best_name(glaxnimate::model::DocumentNode* node, const QString& suggestion={}) const;

    model::Assets* assets() const;

    model::CompGraph& comp_graph();

    void stretch_time(qreal multiplier);

    int add_pending_asset(const QString& name, const QUrl& url);
    int add_pending_asset(const QString& name, const QByteArray& data);
    int add_pending_asset(const model::PendingAsset& asset);
    std::vector<model::PendingAsset> pending_assets();
    void mark_asset_loaded(int pending_id);
    void clear_pending_assets();


signals:
    void filename_changed(const QString& n);
    void current_time_changed(FrameTime t);
    void record_to_keyframe_changed(bool r);
    void graphics_invalidated();

private:
    Object* assets_obj() const;
    void decrease_node_name(const QString& old_name);
    void increase_node_name(const QString& new_name);

private:
    class Private;
    friend DocumentNode;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::model
