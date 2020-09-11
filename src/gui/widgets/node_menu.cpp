#include "node_menu.hpp"
#include "model/layers/layer.hpp"
#include "model/shapes/group.hpp"
#include "command/structure_commands.hpp"
#include "command/animation_commands.hpp"
#include "command/property_commands.hpp"


namespace {

class ResetTransform
{
public:
    void operator()() const
    {
        doc->add_command(new command::SetMultipleAnimated(
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
        act->setIcon(node->docnode_group_icon());
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

} // namespace


NodeMenu::NodeMenu(model::DocumentNode* node, QWidget* parent)
    : QMenu(node->object_name(), parent)
{
    setIcon(node->docnode_icon());
    addSection(node->docnode_icon(), node->object_name());

    if ( auto lay = qobject_cast<model::Layer*>(node) )
    {
        addAction(QIcon::fromTheme("edit-delete-remove"), tr("Delete"), this, [lay]{
            lay->add_command(new command::DeleteCommand(lay));
        });

        addAction(QIcon::fromTheme("edit-duplicate"), tr("Duplicate"), this, [lay]{
            lay->add_command(new command::DuplicateCommand(lay));
        });

        addAction(QIcon::fromTheme("transform-move"), tr("Reset Transform"), this,
            ResetTransform{lay->document(), lay->transform.get()}
        );

        QMenu* menu_parent = new QMenu(tr("Parent"), this);
        menu_parent->setIcon(QIcon::fromTheme("go-parent-folder"));
        QActionGroup* group_parent = new QActionGroup(this);
        auto layparent = lay->parent.get();
        action_for_node(nullptr, layparent, menu_parent, group_parent);
        for ( const auto& other_lay : lay->composition()->layers )
            if ( other_lay.get() != lay )
                action_for_node(other_lay.get(), layparent, menu_parent, group_parent);
        connect(menu_parent, &QMenu::triggered, this, [lay](QAction* act){
            lay->add_command(
                new command::SetPropertyValue(
                    &lay->parent,
                    lay->parent.value(),
                    act->data(),
                    true
                )
            );
        });
        addAction(menu_parent->menuAction());
    }
    else if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
    {
        addAction(QIcon::fromTheme("edit-delete-remove"), tr("Delete"), this, [shape]{
            shape->add_command(new command::DeleteCommand(shape));
        });

        addAction(QIcon::fromTheme("edit-duplicate"), tr("Duplicate"), this, [shape]{
            shape->add_command(new command::DuplicateCommand(shape));
        });

        if ( auto group = qobject_cast<model::Group*>(shape) )
        {
            addAction(QIcon::fromTheme("transform-move"), tr("Reset Transform"), this,
                ResetTransform{group->document(), group->transform.get()}
            );
        }
    }
}

