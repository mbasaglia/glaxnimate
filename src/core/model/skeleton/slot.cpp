#include "slot.hpp"

#include <QPainter>

#include "skeleton_p.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::SkinSlot)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::SkinAttachment)

QIcon glaxnimate::model::SkinSlot::tree_icon() const
{
    return QIcon::fromTheme("link");
}

void glaxnimate::model::SkinSlot::on_skeleton_changed(model::Skeleton* old_skel, model::Skeleton* new_skel)
{
    if ( old_skel )
        old_skel->d->skin_slots.erase(this);
    if ( new_skel )
        new_skel->d->skin_slots.insert(this);

    for ( const auto& attach : attachments )
        attach->on_skeleton_changed(old_skel, new_skel);
}

glaxnimate::model::SkinSlot * glaxnimate::model::SkinAttachment::slot() const
{
    return qobject_cast<SkinSlot*>(docnode_parent());
}

QIcon glaxnimate::model::SkinAttachment::tree_icon() const
{
    return QIcon::fromTheme("link");
}

QIcon glaxnimate::model::SkinAttachment::instance_icon() const
{
    return tree_icon();
}

QString glaxnimate::model::SkinAttachment::type_name_human() const
{
    return tr("Attachment");
}

std::vector<glaxnimate::model::DocumentNode*> glaxnimate::model::SkinSlot::valid_attachments() const
{
    return attachments.valid_reference_values(true);
}

bool glaxnimate::model::SkinSlot::is_valid_attachment(glaxnimate::model::DocumentNode* node) const
{
    return attachments.is_valid_reference_value(node, true);
}

void glaxnimate::model::SkinAttachment::on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent)
{
    model::Skeleton* old_skel = nullptr;
    if ( auto slot = qobject_cast<SkinSlot*>(old_parent) )
        old_skel = slot->skeleton();

    model::Skeleton* new_skel = nullptr;
    if ( auto slot = qobject_cast<SkinSlot*>(new_parent) )
        new_skel = slot->skeleton();

    if ( old_skel != new_skel )
        on_skeleton_changed(old_skel, new_skel);
}

void glaxnimate::model::SkinAttachment::on_skeleton_changed(model::Skeleton* old_skel, model::Skeleton* new_skel)
{
    if ( old_skel )
        old_skel->d->attachments.erase(this);

    if ( new_skel )
        new_skel->d->attachments.insert(this);
}

glaxnimate::model::VisualNode * glaxnimate::model::SkinAttachment::docnode_group_child(int index) const
{
    auto it = users().begin();
    std::advance(it, index);
    return static_cast<VisualNode*>((*it)->object());
}

int glaxnimate::model::SkinAttachment::docnode_group_child_count() const
{
    return users().size();
}

void glaxnimate::model::SkinAttachment::prepare_painter(QPainter* painter, model::FrameTime t) const
{
    painter->setOpacity(painter->opacity() * slot()->opacity.get_at(t));
}
