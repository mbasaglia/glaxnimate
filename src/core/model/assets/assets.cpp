/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "assets.hpp"
#include "model/document.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::NamedColorList)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::GradientColorsList)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::GradientList)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::BitmapList)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::PrecompositionList)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::FontList)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Assets)


void glaxnimate::model::NamedColorList::on_added(glaxnimate::model::NamedColor* color, int position)
{
    connect(color, &Object::property_changed, this, [position, color, this]{
        emit color_changed(position, color);
    });
    Ctor::on_added(color, position);
    emit color_added(position, color);
}

void glaxnimate::model::NamedColorList::on_removed(glaxnimate::model::NamedColor* color, int position)
{
    disconnect(color, nullptr, this, nullptr);
    Ctor::on_removed(color, position);
    emit color_removed(position, color);
}

QIcon glaxnimate::model::NamedColorList::tree_icon() const
{
    return QIcon::fromTheme("paint-swatch");
}


QIcon glaxnimate::model::GradientColorsList::tree_icon() const
{
    return QIcon::fromTheme("paint-gradient-linear");
}

QIcon glaxnimate::model::GradientList::tree_icon() const
{
    return QIcon::fromTheme("gradient");
}


QIcon glaxnimate::model::BitmapList::tree_icon() const
{
    return QIcon::fromTheme("folder-images");
}

QIcon glaxnimate::model::PrecompositionList::tree_icon() const
{
    return QIcon::fromTheme("folder-videos");
}

void glaxnimate::model::PrecompositionList::on_added(glaxnimate::model::Precomposition* obj, int position)
{
    obj->attach();
    document()->comp_graph().add_composition(obj);
    emit docnode_child_add_end(obj, position);
    emit precomp_added(obj, position);
}


void glaxnimate::model::PrecompositionList::on_removed(glaxnimate::model::Precomposition* obj, int position)
{
    obj->detach();
    document()->comp_graph().remove_composition(obj);
    emit docnode_child_remove_end(obj, position);
}

void glaxnimate::model::FontList::on_added ( model::EmbeddedFont* obj, int position )
{
    obj->attach();
    emit docnode_child_add_end(obj, position);
    emit font_added(obj);
}


glaxnimate::model::NamedColor* glaxnimate::model::Assets::add_color(const QColor& color, const QString& name)
{
    auto ptr = std::make_unique<glaxnimate::model::NamedColor>(document());
    ptr->color.set(color);
    ptr->name.set(name);
    auto raw = ptr.get();
    push_command(new command::AddObject(&colors->values, std::move(ptr), colors->values.size()));
    return raw;
}

glaxnimate::model::Bitmap * glaxnimate::model::Assets::add_image_file(const QString& filename, bool embed)
{
    auto image = std::make_unique<glaxnimate::model::Bitmap>(document());
    image->filename.set(filename);
    if ( image->pixmap().isNull() )
        return nullptr;
    image->embed(embed);
    auto ptr = image.get();
    push_command(new command::AddObject(&images->values, std::move(image), images->values.size()));
    return ptr;
}

glaxnimate::model::Bitmap * glaxnimate::model::Assets::add_image(const QImage& qimage, const QString& store_as)
{
    auto image = std::make_unique<glaxnimate::model::Bitmap>(document());
    image->set_pixmap(qimage, store_as);
    auto ptr = image.get();
    push_command(new command::AddObject(&images->values, std::move(image), images->values.size()));
    return ptr;
}

glaxnimate::model::GradientColors* glaxnimate::model::Assets::add_gradient_colors(int index)
{
    glaxnimate::model::GradientColors *ptr = new glaxnimate::model::GradientColors(document());
    ptr->name.set(ptr->type_name_human());
    push_command(new command::AddObject(&gradient_colors->values, std::unique_ptr<glaxnimate::model::GradientColors>(ptr), index));
    return ptr;
}

glaxnimate::model::Gradient* glaxnimate::model::Assets::add_gradient(int index)
{
    glaxnimate::model::Gradient *ptr = new glaxnimate::model::Gradient(document());
    ptr->name.set(ptr->type_name_human());
    push_command(new command::AddObject(&gradients->values, std::unique_ptr<glaxnimate::model::Gradient>(ptr), index));
    return ptr;
}

QIcon glaxnimate::model::Assets::tree_icon() const
{
    return QIcon::fromTheme("folder-stash");
}

QIcon glaxnimate::model::Assets::instance_icon() const
{
    return tree_icon();
}

glaxnimate::model::DocumentNode* glaxnimate::model::detail::defs(glaxnimate::model::Document* doc)
{
    return doc->assets();
}

glaxnimate::model::DocumentNode * glaxnimate::model::Assets::docnode_parent() const
{
    return nullptr;
}

int glaxnimate::model::Assets::docnode_child_count() const
{
    return 6;
}

glaxnimate::model::DocumentNode * glaxnimate::model::Assets::docnode_child(int index) const
{
    switch ( index )
    {
        case 0:
            return const_cast<glaxnimate::model::DocumentNode*>(static_cast<const glaxnimate::model::DocumentNode*>(colors.get()));
        case 1:
            return const_cast<glaxnimate::model::DocumentNode*>(static_cast<const glaxnimate::model::DocumentNode*>(images.get()));
        case 2:
            return const_cast<glaxnimate::model::DocumentNode*>(static_cast<const glaxnimate::model::DocumentNode*>(gradient_colors.get()));
        case 3:
            return const_cast<glaxnimate::model::DocumentNode*>(static_cast<const glaxnimate::model::DocumentNode*>(gradients.get()));
        case 4:
            return const_cast<glaxnimate::model::DocumentNode*>(static_cast<const glaxnimate::model::DocumentNode*>(precompositions.get()));
        case 5:
            return const_cast<glaxnimate::model::DocumentNode*>(static_cast<const glaxnimate::model::DocumentNode*>(fonts.get()));
        default:
            return nullptr;
    }
}

int glaxnimate::model::Assets::docnode_child_index(glaxnimate::model::DocumentNode* dn) const
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
    if ( dn == fonts.get() )
        return 5;
    return -1;
}

glaxnimate::model::EmbeddedFont* glaxnimate::model::Assets::add_font(const QByteArray& ttf_data)
{
    auto font = std::make_unique<glaxnimate::model::EmbeddedFont>(document());
    font->data.set(ttf_data);
    if ( auto old = font_by_index(font->database_index()) )
        return old;
    auto ptr = font.get();
    push_command(new command::AddObject(&fonts->values, std::move(font), fonts->values.size()));
    return ptr;
}

glaxnimate::model::EmbeddedFont* glaxnimate::model::Assets::add_font(const CustomFont& custom_font)
{
    if ( auto old = font_by_index(custom_font.database_index()) )
        return old;

    auto font = std::make_unique<glaxnimate::model::EmbeddedFont>(document(), custom_font);
    auto ptr = font.get();
    push_command(new command::AddObject(&fonts->values, std::move(font), fonts->values.size()));
    return ptr;
}


glaxnimate::model::EmbeddedFont * glaxnimate::model::Assets::font_by_index(int database_index) const
{
    for ( const auto& font : fonts->values )
        if ( font->database_index() == database_index )
            return font.get();
    return nullptr;
}
