#include "defs.hpp"
#include "model/document.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Defs)

std::vector<model::ReferenceTarget *> model::Defs::valid_brush_styles() const
{
    std::vector<model::ReferenceTarget *> res;
    res.reserve(colors.size()+1);
    res.push_back(nullptr);
    for ( const auto& c : colors )
        res.push_back(c.get());
    return res;
}

bool model::Defs::is_valid_brush_style(model::ReferenceTarget* style) const
{
    if ( style == nullptr )
        return true;

    for ( const auto& c : colors )
        if ( c.get() == style )
            return true;
    return false;
}

std::vector<model::ReferenceTarget *> model::Defs::valid_images() const
{
    std::vector<model::ReferenceTarget *> res;
    res.reserve(images.size());
    for ( const auto& c : images )
        res.push_back(c.get());
    return res;
}

bool model::Defs::is_valid_image(model::ReferenceTarget* style) const
{
    for ( const auto& c : images )
        if ( c.get() == style )
            return true;
    return false;
}

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

model::ReferenceTarget* model::Defs::find_by_uuid ( const QUuid& n ) const
{
    for ( const auto& c : colors )
        if ( c->uuid.get() == n )
            return c.get();
    for ( const auto& c : images )
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
