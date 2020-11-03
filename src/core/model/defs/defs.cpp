#include "defs.hpp"
#include "model/document.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Defs)

void model::Defs::on_color_added(model::NamedColor* color, int position)
{
    connect(color, &Object::property_changed, this, [position, color, this]{
        emit color_changed(position, color);
    });
    color->attach();
    emit color_added(position, color);
}

void model::Defs::on_color_removed(model::NamedColor* color, int position)
{
    disconnect(color, nullptr, this, nullptr);
    color->detach();
    emit color_removed(position);
}

void model::Defs::on_gradient_colors_added(model::GradientColors* color)
{
    color->attach();
    emit gradient_add_end(color);
}

void model::Defs::on_gradient_colors_removed(model::GradientColors* color)
{
    color->detach();
    emit gradient_remove_end(color);
}

model::ReferenceTarget* model::Defs::find_by_uuid ( const QUuid& n ) const
{
    for ( const auto& c : gradient_colors )
        if ( c->uuid.get() == n )
            return c.get();

    for ( const auto& c : gradients )
        if ( c->uuid.get() == n )
            return c.get();

    for ( const auto& c : images )
        if ( c->uuid.get() == n )
            return c.get();

    for ( const auto& c : colors )
        if ( c->uuid.get() == n )
            return c.get();

    return nullptr;
}

model::NamedColor* model::Defs::add_color(const QColor& color, const QString& name)
{
    auto ptr = std::make_unique<model::NamedColor>(document());
    ptr->color.set(color);
    ptr->name.set(name);
    auto raw = ptr.get();
    push_command(new command::AddObject(&colors, std::move(ptr), colors.size()));
    return raw;
}

model::Bitmap * model::Defs::add_image_file(const QString& filename, bool embed)
{
    auto image = std::make_unique<model::Bitmap>(document());
    image->filename.set(filename);
    if ( image->pixmap().isNull() )
        return nullptr;
    image->embed(embed);
    auto ptr = image.get();
    push_command(new command::AddObject(&images, std::move(image), images.size()));
    return ptr;
}

void model::Defs::on_added(model::Asset* def)
{
    def->attach();
}

void model::Defs::on_removed(model::Asset* def)
{
    def->detach();
}

model::Bitmap * model::Defs::add_image(const QImage& qimage, const QString& store_as)
{
    auto image = std::make_unique<model::Bitmap>(document());
    image->set_pixmap(qimage, store_as);
    auto ptr = image.get();
    push_command(new command::AddObject(&images, std::move(image), images.size()));
    return ptr;
}

model::GradientColors* model::Defs::add_gradient_colors(int index)
{
    model::GradientColors *ptr = new model::GradientColors(document());
    ptr->name.set(ptr->type_name_human());
    push_command(new command::AddObject(&gradient_colors, std::unique_ptr<model::GradientColors>(ptr), index));
    return ptr;
}

model::Gradient* model::Defs::add_gradient(int index)
{
    model::Gradient *ptr = new model::Gradient(document());
    ptr->name.set(ptr->type_name_human());
    push_command(new command::AddObject(&gradients, std::unique_ptr<model::Gradient>(ptr), index));
    return ptr;
}
