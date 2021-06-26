#pragma once

#include <set>

#include "model/document.hpp"

class QAction;

namespace settings {

class DocumentTemplate
{
    Q_GADGET

public:
    DocumentTemplate(const QString& filename, bool* loaded);

    QSize size() const;

    model::FrameTime duration() const;

    QString name() const;

    QString long_name() const;

    float fps() const;

    std::unique_ptr<model::Document> create(bool* ok) const;

    bool operator<(const DocumentTemplate& oth) const;

    static QString name_template(model::Document* document);

    QString aspect_ratio() const;
    static QString aspect_ratio(const QSize& size);

private:
    QString filename;
    std::unique_ptr<model::Document> document;

    std::unique_ptr<model::Document> load(bool* ok) const;
};

class DocumentTemplates : public QObject
{
    Q_OBJECT

public:
    const std::vector<DocumentTemplate>& templates() const;

    void load();

    bool save_as_template(model::Document* document);

    static DocumentTemplates& instance();

    QAction* create_action(const DocumentTemplate& templ, QObject *parent = nullptr);

signals:
    void loaded(const std::vector<DocumentTemplate>& templates);
    void create_from(const DocumentTemplate& templ);

private:
    DocumentTemplates();

    std::vector<DocumentTemplate> templates_;
    std::set<QString> names;

};

} // namespace settings

