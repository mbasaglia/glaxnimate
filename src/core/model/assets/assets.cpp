#include "assets.hpp"
#include "model/document.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(model::NamedColorList)
GLAXNIMATE_OBJECT_IMPL(model::GradientColorsList)
GLAXNIMATE_OBJECT_IMPL(model::GradientList)
GLAXNIMATE_OBJECT_IMPL(model::BitmapList)
GLAXNIMATE_OBJECT_IMPL(model::PrecompositionList)
GLAXNIMATE_OBJECT_IMPL(model::Assets)


void model::NamedColorList::on_added(model::NamedColor* color, int position)
{
    connect(color, &Object::property_changed, this, [position, color, this]{
        emit color_changed(position, color);
    });
    Ctor::on_added(color, position);
    emit color_added(position, color);
}

void model::NamedColorList::on_removed(model::NamedColor* color, int position)
{
    disconnect(color, nullptr, this, nullptr);
    Ctor::on_removed(color, position);
    emit color_removed(position, color);
}

QIcon model::NamedColorList::tree_icon() const
{
    return QIcon::fromTheme("paint-swatch");
}


QIcon model::GradientColorsList::tree_icon() const
{
    return QIcon::fromTheme("paint-gradient-linear");
}

QIcon model::GradientList::tree_icon() const
{
    return QIcon::fromTheme("gradient");
}


QIcon model::BitmapList::tree_icon() const
{
    return QIcon::fromTheme("folder-images");
}

QIcon model::PrecompositionList::tree_icon() const
{
    return QIcon::fromTheme("folder-videos");
}

void model::PrecompositionList::on_added(model::Precomposition* obj, int position)
{
    obj->attach();
    document()->comp_graph().add_composition(obj);
    emit docnode_child_add_end(obj, position);
    emit precomp_added(obj, position);
}


void model::PrecompositionList::on_removed(model::Precomposition* obj, int position)
{
    obj->detach();
    document()->comp_graph().remove_composition(obj);
    emit docnode_child_remove_end(obj, position);
}


model::NamedColor* model::Assets::add_color(const QColor& color, const QString& name)
{
    auto ptr = std::make_unique<model::NamedColor>(document());
    ptr->color.set(color);
    ptr->name.set(name);
    auto raw = ptr.get();
    push_command(new command::AddObject(&colors->values, std::move(ptr), colors->values.size()));
    return raw;
}

model::Bitmap * model::Assets::add_image_file(const QString& filename, bool embed)
{
    auto image = std::make_unique<model::Bitmap>(document());
    image->filename.set(filename);
    if ( image->pixmap().isNull() )
        return nullptr;
    image->embed(embed);
    auto ptr = image.get();
    push_command(new command::AddObject(&images->values, std::move(image), images->values.size()));
    return ptr;
}

model::Bitmap * model::Assets::add_image(const QImage& qimage, const QString& store_as)
{
    auto image = std::make_unique<model::Bitmap>(document());
    image->set_pixmap(qimage, store_as);
    auto ptr = image.get();
    push_command(new command::AddObject(&images->values, std::move(image), images->values.size()));
    return ptr;
}

model::GradientColors* model::Assets::add_gradient_colors(int index)
{
    model::GradientColors *ptr = new model::GradientColors(document());
    ptr->name.set(ptr->type_name_human());
    push_command(new command::AddObject(&gradient_colors->values, std::unique_ptr<model::GradientColors>(ptr), index));
    return ptr;
}

model::Gradient* model::Assets::add_gradient(int index)
{
    model::Gradient *ptr = new model::Gradient(document());
    ptr->name.set(ptr->type_name_human());
    push_command(new command::AddObject(&gradients->values, std::unique_ptr<model::Gradient>(ptr), index));
    return ptr;
}

QIcon model::Assets::tree_icon() const
{
    return QIcon::fromTheme("folder-stash");
}

QIcon model::Assets::instance_icon() const
{
    return tree_icon();
}

model::DocumentNode* model::detail::defs(model::Document* doc)
{
    return doc->assets();
}

model::DocumentNode * model::Assets::docnode_parent() const
{
    return nullptr;
}

int model::Assets::docnode_child_count() const
{
    return 5;
}

model::DocumentNode * model::Assets::docnode_child(int index) const
{
    switch ( index )
    {
        case 0:
            return const_cast<model::DocumentNode*>(static_cast<const model::DocumentNode*>(colors.get()));
        case 1:
            return const_cast<model::DocumentNode*>(static_cast<const model::DocumentNode*>(images.get()));
        case 2:
            return const_cast<model::DocumentNode*>(static_cast<const model::DocumentNode*>(gradient_colors.get()));
        case 3:
            return const_cast<model::DocumentNode*>(static_cast<const model::DocumentNode*>(gradients.get()));
        case 4:
            return const_cast<model::DocumentNode*>(static_cast<const model::DocumentNode*>(precompositions.get()));
        default:
            return nullptr;
    }
}

int model::Assets::docnode_child_index(model::DocumentNode* dn) const
{
    if ( dn == colors.get() )
        return 0;
    if ( dn == images.get() )
        return 1;
    if ( dn == gradient_colors.get() )
        return 2;
    if ( dn == gradients.get() )
        return 3;
    if ( dn == precompositions.get() )
        return 4;
    return -1;
}



