#include "node_menu.hpp"

#include <QDesktopServices>

#include "model/shapes/group.hpp"
#include "command/structure_commands.hpp"
#include "command/animation_commands.hpp"
#include "command/property_commands.hpp"
#include "command/shape_commands.hpp"
#include "widgets/dialogs/shape_parent_dialog.hpp"
#include "model/shapes/image.hpp"

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

QAction* action_for_node(model::ReferenceTarget* node, model::ReferenceTarget* selected, QMenu* parent, QActionGroup* group)
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
    model::ShapeElement* node,
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

QMenu* menu_ref_property(const QIcon& icon, const QString& text, QWidget* parent, model::ReferencePropertyBase* prop)
{
    QMenu* menu = new QMenu(text, parent);
    menu->setIcon(icon);
    QActionGroup* group = new QActionGroup(parent);
    auto value = prop->value().value<model::ReferenceTarget*>();
    for ( auto other_lay : prop->valid_options() )
        action_for_node(other_lay, value, menu, group);

    QObject::connect(menu, &QMenu::triggered, parent, [prop](QAction* act){
        prop->object()->push_command(
            new command::SetPropertyValue(
                prop,
                prop->value(),
                act->data(),
                true
            )
        );
    });

    return menu;
}

} // namespace



NodeMenu::NodeMenu(model::DocumentNode* node, GlaxnimateWindow* window, QWidget* parent)
    : QMenu(node->object_name(), parent)
{
    setIcon(node->docnode_icon());
    addSection(node->docnode_icon(), node->object_name());

    if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
    {
        addAction(QIcon::fromTheme("edit-delete-remove"), tr("Delete"), this, [shape]{
            shape->push_command(new command::RemoveShape(shape, shape->owner()));
        });

        addAction(QIcon::fromTheme("edit-duplicate"), tr("Duplicate"), this, [shape]{
            shape->push_command(command::duplicate_shape(shape));
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

        if ( auto lay = qobject_cast<model::Layer*>(shape) )
        {
            addAction(menu_ref_property(QIcon::fromTheme("go-parent-folder"), tr("Parent"), this, &lay->parent)->menuAction());
        }
        else if ( auto image = qobject_cast<model::Image*>(shape) )
        {
            addSeparator();

            addAction(menu_ref_property(QIcon::fromTheme("folder-pictures"), tr("Image"), this, &image->image)->menuAction());

            addAction(QIcon::fromTheme("mail-attachment-symbolic"), tr("Embed"), this, [image]{
                if ( image->image.get() )
                    image->image->embed(true);
            })->setEnabled(image->image.get() && !image->image->embedded());

            addAction(QIcon::fromTheme("editimage"), tr("Open with External Application"), this, [image, window]{
                if ( image->image.get() )
                {
                    if ( !QDesktopServices::openUrl(image->image->to_url()) )
                        window->warning(tr("Could not find suitable application, check your system settings."));
                }
            })->setEnabled(image->image.get());


            addAction(QIcon::fromTheme("document-open"), tr("From File..."), this, [image, window]{
                auto bmp = image->image.get();
                QString filename = window->get_open_image_file(tr("Update Image"), bmp ? bmp->file_info().absolutePath() : "");
                if ( filename.isEmpty() )
                    return;

                image->document()->undo_stack().beginMacro(tr("Update Image"));
                if ( bmp )
                {
                    bmp->data.set_undoable(QByteArray());
                    bmp->filename.set_undoable(filename);
                }
                else
                {
                    auto img = image->document()->defs()->add_image(filename, false);
                    if ( img )
                        image->image.set_undoable(QVariant::fromValue(img));
                }


                image->document()->undo_stack().endMacro();
            });
        }
    }
}
