/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QtGlobal>

namespace glaxnimate::model {

class Document;
class Composition;
class DocumentNode;

class Visitor
{
public:
    virtual ~Visitor() {}

    void visit(model::Document* doc, model::Composition* main, bool skip_locked = false);
    void visit(model::DocumentNode* node, bool skip_locked = false);

private:
    virtual void on_visit(model::DocumentNode* node) = 0;
    virtual void on_visit_end(model::DocumentNode* node) { Q_UNUSED(node) }
    virtual void on_visit_document(model::Document* document, model::Composition* main) { Q_UNUSED(document) Q_UNUSED(main) }
    virtual void on_visit_document_end(model::Document* document, model::Composition* main) { Q_UNUSED(document) Q_UNUSED(main) }
};

} // namespace glaxnimate::model
