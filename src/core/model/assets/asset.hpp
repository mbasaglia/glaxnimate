/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unordered_set>

#include "model/document_node.hpp"
#include "asset_base.hpp"
#include "model/property/reference_property.hpp"

namespace glaxnimate::model {

class Asset : public DocumentNode, public AssetBase
{
    Q_OBJECT

public:
    using DocumentNode::DocumentNode;

Q_SIGNALS:
    void users_changed();

protected:
    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QIcon tree_icon() const override { return instance_icon(); }
};



} // namespace glaxnimate::model
