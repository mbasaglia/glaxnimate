/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QtGlobal>

namespace glaxnimate::model {

class Document;
class DocumentNode;

class Visitor
{
public:
    virtual ~Visitor() {}

    void visit(model::Document* doc, bool skip_locked = false);
    void visit(model::DocumentNode* node, bool skip_locked = false);

private:
    virtual void on_visit(model::DocumentNode* node) = 0;
    virtual void on_visit_end(model::DocumentNode* node) { Q_UNUSED(node) }
    virtual void on_visit(model::Document* document) { Q_UNUSED(document) }
    virtual void on_visit_end(model::Document* document) { Q_UNUSED(document) }
};

} // namespace glaxnimate::model
