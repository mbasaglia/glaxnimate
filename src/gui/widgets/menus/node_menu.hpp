/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QMenu>

#include "model/document_node.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"

namespace glaxnimate::gui {


class NodeMenu : public QMenu
{
    Q_OBJECT
public:
    NodeMenu(model::DocumentNode* node, GlaxnimateWindow* window, QWidget* parent);
};

} // namespace glaxnimate::gui
