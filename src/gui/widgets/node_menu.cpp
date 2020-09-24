#include "node_menu.hpp"
#include "model/layers/layer.hpp"
#include "model/shapes/group.hpp"
#include "command/structure_commands.hpp"
#include "command/animation_commands.hpp"
#include "command/property_commands.hpp"
#include "command/shape_commands.hpp"
#include "widgets/dialogs/shape_parent_dialog.hpp"

namespace {

class ResetTransform
{
public:
    void operator()() const
    {
        doc->push_command(new command::SetMultipleAnimated(
            NodeMenu::tr("ResetTransform"),
            true,
            {
                &trans->position,
                &trans->scale,
                &trans->rotation
            },
            trans->anchor_point.get(),
            QVector2D(1, 1),
            0
        ));
    }

    model::Document* doc;
    model::Transform* trans;
};

QAction* action_for_node(model::DocumentNode* node, model::DocumentNode* selected, QMenu* parent, QActionGroup* group)
{
    QAction* act = new QAction(parent);
    if ( node )
    {
        act->setIcon(node->reftarget_icon());
        act->setText(node->object_name());
    }
    else
    {
        act->setIcon(QIcon::fromTheme("edit-none"));
        act->setText(NodeMenu::tr("None"));
    }

    act->setActionGroup(group);
    act->setCheckable(true);
    act->setChecked(node == selected);
    act->setData(QVariant::fromValue(node));
    parent->addAction(act);
    return act;
}

void move_action(
    NodeMenu* menu,
    model::DocumentNode* node,
    command::ReorderCommand::SpecialPosition pos
)
{
    QIcon icon;
    QString label;

    switch ( pos )
    {
        case command::ReorderCommand::MoveTop:
            icon = QIcon::fromTheme("layer-top");
            label = NodeMenu::tr("Move to Top");
            break;
        case command::ReorderCommand::MoveUp:
            icon = QIcon::fromTheme("layer-raise");
            label = NodeMenu::tr("Raise");
            break;
        case command::ReorderCommand::MoveDown:
            icon = QIcon::fromTheme("layer-lower");
            label = NodeMenu::tr("Lower");
            break;
        case command::ReorderCommand::MoveBottom:
            icon = QIcon::fromTheme("layer-bottom");
            label = NodeMenu::tr("Move to Bottom");
            break;
    }

    QAction* act = menu->addAction(icon, label, menu, [node, pos]{
        node->push_command(new command::ReorderCommand(node, pos));
    });

    int position = pos;
    if ( !command::ReorderCommand::resolve_position(node, position) )
        act->setEnabled(false);
}


} // namespace



NodeMenu::NodeMenu(model::DocumentNode* node, GlaxnimateWindow* window, QWidget* parent)
    : QMenu(node->object_name(), parent)
{
    setIcon(node->docnode_icon());
    addSection(node->docnode_icon(), node->object_name());

    if ( auto lay = qobject_cast<model::Layer*>(node) )
    {
        addAction(QIcon::fromTheme("edit-delete-remove"), tr("Delete"), this, [lay]{
            lay->push_command(new command::DeleteCommand(lay));
        });

        addAction(QIcon::fromTheme("edit-duplicate"), tr("Duplicate"), this, [lay]{
            lay->push_command(new command::DuplicateCommand(lay));
        });

        addSeparator();

        addAction(QIcon::fromTheme("transform-move"), tr("Reset Transform"), this,
            ResetTransform{lay->document(), lay->transform.get()}
        );

        addSeparator();

        QMenu* menu_parent = new QMenu(tr("Parent"), this);
        menu_parent->setIcon(QIcon::fromTheme("go-parent-folder"));
        QActionGroup* group_parent = new QActionGroup(this);
        auto layparent = lay->parent.get();
        action_for_node(nullptr, layparent, menu_parent, group_parent);
        for ( const auto& other_lay : lay->composition()->layers )
            if ( other_lay.get() != lay )
                action_for_node(other_lay.get(), layparent, menu_parent, group_parent);
        connect(menu_parent, &QMenu::triggered, this, [lay](QAction* act){
            lay->push_command(
                new command::SetPropertyValue(
                    &lay->parent,
                    lay->parent.value(),
                    act->data(),
                    true
                )
            );
        });
        addAction(menu_parent->menuAction());

        addSeparator();

        move_action(this, lay, command::ReorderCommand::MoveTop);
        move_action(this, lay, command::ReorderCommand::MoveUp);
        move_action(this, lay, command::ReorderCommand::MoveDown);
        move_action(this, lay, command::ReorderCommand::MoveBottom);

    }
    else if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
    {
        addAction(QIcon::fromTheme("edit-delete-remove"), tr("Delete"), this, [shape]{
            shape->push_command(new command::DeleteCommand(shape));
        });

        addAction(QIcon::fromTheme("edit-duplicate"), tr("Duplicate"), this, [shape]{
            shape->push_command(new command::DuplicateCommand(shape));
        });

        addSeparator();
        if ( auto group = qobject_cast<model::Group*>(shape) )
        {
            addAction(QIcon::fromTheme("transform-move"), tr("Reset Transform"), this,
                ResetTransform{group->document(), group->transform.get()}
            );
        }

        addSeparator();

        move_action(this, shape, command::ReorderCommand::MoveTop);
        move_action(this, shape, command::ReorderCommand::MoveUp);
        move_action(this, shape, command::ReorderCommand::MoveDown);
        move_action(this, shape, command::ReorderCommand::MoveBottom);

        addAction(QIcon::fromTheme("selection-move-to-layer-above"), tr("Move to..."), this, [shape, window]{
            if ( auto parent = ShapeParentDialog(window->model(), window).get_shape_parent() )
            {
                if ( shape->owner() != parent )
                    shape->push_command(new command::MoveShape(shape, shape->owner(), parent, parent->size()));
            }
        });
    }
}

