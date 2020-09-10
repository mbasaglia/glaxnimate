#pragma once

#include <QMenu>

#include "model/document_node.hpp"

class NodeMenu : public QMenu
{
    Q_OBJECT
public:
    NodeMenu(model::DocumentNode* node, QWidget* parent);
};
