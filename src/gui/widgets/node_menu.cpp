#include "node_menu.hpp"

#include <QDesktopServices>

#include "model/shapes/group.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/repeater.hpp"

#include "model/assets/assets.hpp"

#include "command/structure_commands.hpp"
#include "command/animation_commands.hpp"
#include "command/property_commands.hpp"
#include "command/shape_commands.hpp"
#include "command/undo_macro_guard.hpp"

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
        act->setIcon(node->instance_icon());
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
    auto value = prop->value().value<model::DocumentNode*>();
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

template<class ToType>
void convert_group_callback(int, ToType*){}

template<class Callback, class ToType>
void convert_group_callback(const Callback& callback, ToType* layer)
{
    callback(layer);
}

template<class ToType, class Callback = int>
class ConvertGroupType
{
public:
    ConvertGroupType(model::Group* from, Callback callback = {})
        : from(from), owner(static_cast<model::ShapeListProperty*>(from->docnode_parent()->get_property("shapes"))),
        callback(std::move(callback))
    {}

    void operator()() const
    {
        convert();
    }

    void convert() const
    {
        auto to = new ToType(from->document());
        std::unique_ptr<model::ShapeElement> uto(to);

        for ( auto prop : to->properties() )
        {
            if ( prop != &to->uuid && prop != &to->shapes )
            {
                if ( auto src = from->get_property(prop->name()) )
                    prop->assign_from(src);
            }
        }

        command::UndoMacroGuard guard(NodeMenu::tr("Convert %1 to %2").arg(from->name.get()).arg(to->type_name_human()), from->document());
        from->push_command(new command::AddObject(owner, std::move(uto), owner->index_of(from)));
        std::vector<model::ShapeElement*> shapes;
        shapes.reserve(from->shapes.size());
        for ( const auto& shape : from->shapes )
            shapes.push_back(shape.get());
        for ( std::size_t i = 0; i < shapes.size(); i++ )
            from->push_command(new command::MoveObject(shapes[i], &from->shapes, &to->shapes, i));
        from->push_command(new command::RemoveObject<model::ShapeElement>(from, owner));
        convert_group_callback(callback, to);
    }

private:
    model::Group* from;
    model::ShapeListProperty* owner;
    Callback callback;
};

void togglable_action(QMenu* menu, model::Property<bool>* prop, const QString& icon, const QString& label)
{
    auto action = menu->addAction(QIcon::fromTheme(icon), label, menu, [prop](bool value){
        prop->set_undoable(value);
    });
    action->setCheckable(true);
    action->setChecked(prop->get());
}

template<class T>
void add_child_action(QMenu* menu, model::Group* group)
{
    menu->addAction(T::static_tree_icon(), T::static_type_name_human(), [group]{
        auto object = std::make_unique<T>(group->document());
        group->document()->set_best_name(object.get());
        group->push_command(new command::AddObject<model::ShapeElement>(&group->shapes, std::move(object), 0));
    });
}

void actions_group(QMenu* menu, GlaxnimateWindow* window, model::Group* group)
{
    QMenu* menu_add = new QMenu(NodeMenu::tr("Add"), menu);
    menu_add->setIcon(QIcon::fromTheme("list-add"));
    menu->addAction(menu_add->menuAction());
    add_child_action<model::Fill>(menu_add, group);
    add_child_action<model::Stroke>(menu_add, group);
    menu_add->addSeparator();
    add_child_action<model::Repeater>(menu_add, group);

    menu->addSeparator();

    menu->addAction(QIcon::fromTheme("transform-move"), NodeMenu::tr("Reset Transform"), menu,
        ResetTransform{group->document(), group->transform.get()}
    );


    model::Layer* lay = qobject_cast<model::Layer*>(group);
    if ( lay )
    {
        menu->addAction(QIcon::fromTheme("timeline-use-zone-on"), NodeMenu::tr("Span All Frames"), menu, [lay]{
            command::UndoMacroGuard guard(NodeMenu::tr("Span All Frames"), lay->document());
            lay->animation->first_frame.set_undoable(
                lay->document()->main()->animation->first_frame.get()
            );
            lay->animation->last_frame.set_undoable(
                lay->document()->main()->animation->last_frame.get()
            );
        });

        menu->addSeparator();
        menu->addAction(menu_ref_property(QIcon::fromTheme("go-parent-folder"), NodeMenu::tr("Parent"), menu, &lay->parent)->menuAction());
        menu->addAction(QIcon::fromTheme("object-group"), NodeMenu::tr("Convert to Group"), menu, ConvertGroupType<model::Group>(lay));
        menu->addAction(QIcon::fromTheme("component"), NodeMenu::tr("Precompose"), menu, [window, lay]{
            window->shape_to_precomposition(lay);
        });

        if ( !lay->mask->has_mask() )
        {
            menu->addAction(QIcon::fromTheme("path-mask-edit"), NodeMenu::tr("Convert to Mask"), menu, [lay]{
                lay->mask->mask.set_undoable(true);
            });
        }
        else
        {
            menu->addAction(QIcon::fromTheme("path-mask-edit"), NodeMenu::tr("Remove Mask"), menu, [lay]{
                lay->mask->mask.set_undoable(false);
            });
        }
    }
    else
    {
        menu->addSeparator();

        menu->addAction(QIcon::fromTheme("folder"), NodeMenu::tr("Convert to Layer"), menu, ConvertGroupType<model::Layer>(group));
        auto callback = [](model::Layer* lay){
            lay->mask->mask.set_undoable(true);
        };
        menu->addAction(QIcon::fromTheme("path-mask-edit"), NodeMenu::tr("Convert to Mask"), menu,
                        ConvertGroupType<model::Layer, decltype(callback)>(group, callback));
    }
}

void actions_image(QMenu* menu, GlaxnimateWindow* window, model::Image* image)
{
    menu->addAction(QIcon::fromTheme("transform-move"), NodeMenu::tr("Reset Transform"), menu,
        ResetTransform{image->document(), image->transform.get()}
    );

    menu->addSeparator();

    menu->addAction(menu_ref_property(QIcon::fromTheme("folder-pictures"), NodeMenu::tr("Image"), menu, &image->image)->menuAction());

    menu->addAction(QIcon::fromTheme("mail-attachment-symbolic"), NodeMenu::tr("Embed"), menu, [image]{
        if ( image->image.get() )
            image->image->embed(true);
    })->setEnabled(image->image.get() && !image->image->embedded());

    menu->addAction(QIcon::fromTheme("editimage"), NodeMenu::tr("Open with External Application"), menu, [image, window]{
        if ( image->image.get() )
        {
            if ( !QDesktopServices::openUrl(image->image->to_url()) )
                window->warning(NodeMenu::tr("Could not find suitable application, check your system settings."));
        }
    })->setEnabled(image->image.get());


    menu->addAction(QIcon::fromTheme("document-open"), NodeMenu::tr("From File..."), menu, [image, window]{
        auto bmp = image->image.get();
        QString filename = window->get_open_image_file(NodeMenu::tr("Update Image"), bmp ? bmp->file_info().absolutePath() : "");
        if ( filename.isEmpty() )
            return;

        command::UndoMacroGuard macro(NodeMenu::tr("Update Image"), image->document());
        if ( bmp )
        {
            bmp->data.set_undoable(QByteArray());
            bmp->filename.set_undoable(filename);
        }
        else
        {
            auto img = image->document()->assets()->add_image_file(filename, false);
            if ( img )
                image->image.set_undoable(QVariant::fromValue(img));
        }
    });

    menu->addAction(QIcon::fromTheme("bitmap-trace"), NodeMenu::tr("Trace Bitmap..."), menu, [image, window]{
        window->trace_dialog(image);
    });
}

void actions_precomp(QMenu* menu, GlaxnimateWindow*, model::PreCompLayer* lay)
{
    menu->addAction(QIcon::fromTheme("transform-move"), NodeMenu::tr("Reset Transform"), menu,
        ResetTransform{lay->document(), lay->transform.get()}
    );

    menu->addAction(QIcon::fromTheme("edit-rename"), NodeMenu::tr("Rename from Composition"), menu, [lay]{
        if ( lay->composition.get() )
            lay->name.set_undoable(lay->composition->object_name());
    });
    menu->addAction(QIcon::fromTheme("archive-extract"), NodeMenu::tr("Decompose"), menu, [lay]{
        command::UndoMacroGuard guard(NodeMenu::tr("Decompose"), lay->document());

        auto comp = lay->composition.get();

        if ( comp )
        {
            int index = lay->owner()->index_of(lay);
            for ( const auto& child : comp->shapes )
            {
                std::unique_ptr<model::ShapeElement> clone(static_cast<model::ShapeElement*>(child->clone().release()));
                clone->refresh_uuid();
                lay->push_command(new command::AddShape(lay->owner(), std::move(clone), ++index));
            }
        }

        lay->push_command(new command::RemoveShape(lay, lay->owner()));

        if ( comp && comp->users().empty() )
            lay->push_command(new command::RemoveObject(comp, &lay->document()->assets()->precompositions->values));
    });
}

} // namespace



NodeMenu::NodeMenu(model::DocumentNode* node, GlaxnimateWindow* window, QWidget* parent)
    : QMenu(node->object_name(), parent)
{
    setIcon(node->tree_icon());
    addSection(node->tree_icon(), node->object_name());

    if ( auto visual = qobject_cast<model::VisualNode*>(node) )
    {
        togglable_action(this, &visual->visible, "view-visible", tr("Visible"));
        togglable_action(this, &visual->locked, "object-locked", tr("Locked"));
        addSeparator();
    }

    if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
    {
        addAction(QIcon::fromTheme("edit-delete-remove"), tr("Delete"), this, [shape]{
            shape->push_command(new command::RemoveShape(shape, shape->owner()));
        });

        addAction(QIcon::fromTheme("edit-duplicate"), tr("Duplicate"), this, [shape, window]{
            auto cmd = command::duplicate_shape(shape);
            shape->push_command(cmd);
            window->set_current_document_node(cmd->object());
        });

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

        addSeparator();

        if ( auto group = qobject_cast<model::Group*>(shape) )
        {
            actions_group(this, window, group);
        }
        else if ( auto image = qobject_cast<model::Image*>(shape) )
        {
            actions_image(this, window, image);
        }
        else if ( auto lay = qobject_cast<model::PreCompLayer*>(shape) )
        {
            actions_precomp(this, window, lay);
        }
    }
    else if ( qobject_cast<model::Composition*>(node) )
    {
        addAction(
            window->create_layer_menu()->menuAction()
        );
    }
}
