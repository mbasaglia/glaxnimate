/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "composition.hpp"
#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "command/object_list_commands.hpp"
#include <QPainter>

using namespace glaxnimate;

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Composition)

QIcon glaxnimate::model::Composition::tree_icon() const
{
    return QIcon::fromTheme("video-x-generic");
}

QString glaxnimate::model::Composition::type_name_human() const
{
    return tr("Composition");
}

bool glaxnimate::model::Composition::remove_if_unused(bool clean_lists)
{
    if ( clean_lists && users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->compositions->values
        ));
        return true;
    }
    return false;
}

glaxnimate::model::DocumentNode * glaxnimate::model::Composition::docnode_parent() const
{
    return document()->assets()->compositions.get();
}


int glaxnimate::model::Composition::docnode_child_index(glaxnimate::model::DocumentNode* dn) const
{
    return shapes.index_of(static_cast<ShapeElement*>(dn));
}

QRectF glaxnimate::model::Composition::local_bounding_rect(FrameTime) const
{
    return rect();
}

QImage glaxnimate::model::Composition::render_image(float time, QSize image_size, const QColor& background) const
{
    QSizeF real_size = size();
    if ( !image_size.isValid() )
        image_size = real_size.toSize();
    QImage image(image_size, QImage::Format_RGBA8888);
    if ( !background.isValid() )
        image.fill(Qt::transparent);
    else
        image.fill(background);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(
        image_size.width() / real_size.width(),
        image_size.height() / real_size.height()
    );
    paint(&painter, time, VisualNode::Render);

    return image;
}

QImage glaxnimate::model::Composition::render_image() const
{
    return render_image(document()->current_time(), size());
}
