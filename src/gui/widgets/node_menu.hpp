#pragma once

#include <QMenu>

#include "model/document_node.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"

class NodeMenu : public QMenu
{
    Q_OBJECT
public:
    NodeMenu(model::DocumentNode* node, GlaxnimateWindow* window, QWidget* parent);
};
