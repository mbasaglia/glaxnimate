#include "skin.hpp"


#include "skeleton_p.hpp"
#include "model/document.hpp"


GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Skin)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::ShapeSkin)


glaxnimate::model::Composition * glaxnimate::model::SkinItem::owner_composition() const
{
    if ( auto skel = skeleton() )
        return skel->owner_composition();
    return nullptr;
}

glaxnimate::model::SkinSlot * glaxnimate::model::SkinItem::slot() const
{
    if ( attachment.get() )
        return attachment->slot();
    return nullptr;
}

void glaxnimate::model::SkinItem::on_parent_changed(model::DocumentNode*, model::DocumentNode* new_parent)
{
    skin_ = qobject_cast<Skin*>(new_parent);
}

glaxnimate::model::Skin * glaxnimate::model::SkinItem::skin() const
{
    return skin_;
}

glaxnimate::model::Skeleton * glaxnimate::model::SkinItem::skeleton() const
{
    return skin_ ? skin_->skeleton() : nullptr;
}

glaxnimate::model::VisualNode * glaxnimate::model::SkinItem::docnode_group_parent() const
{
    return attachment.get();
}

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::SkinItem::valid_slots() const
{
    auto skel = skeleton();
    if ( !skel )
        return {};
    return std::vector<DocumentNode*>(skel->d->attachments.begin(), skel->d->attachments.end());
}

bool glaxnimate::model::SkinItem::is_valid_slot(glaxnimate::model::DocumentNode* node) const
{
    if ( !node )
        return true;
    auto skel = skeleton();
    if ( !skel )
        return false;
    return skel->d->attachments.count(static_cast<SkinAttachment*>(node));
}

QString glaxnimate::model::SkinItem::object_name() const
{
    if ( attachment.get() )
        return attachment->slot()->object_name() + " > " + attachment->object_name();
    return VisualNode::object_name();
}

void glaxnimate::model::SkinItem::on_slot_changed(glaxnimate::model::SkinAttachment*, glaxnimate::model::SkinAttachment*)
{
    emit name_changed(object_name());
}

glaxnimate::model::Skeleton * glaxnimate::model::Skin::skeleton() const
{
    return skeleton_;
}

QIcon glaxnimate::model::Skin::tree_icon() const
{
    return QIcon::fromTheme("skin");
}

QString glaxnimate::model::Skin::type_name_human() const
{
    return tr("Skin");
}

glaxnimate::model::DocumentNode * glaxnimate::model::Skin::docnode_child(int index) const
{
    return items[index];
}

int glaxnimate::model::Skin::docnode_child_count() const
{
    return items.size();
}

int glaxnimate::model::Skin::docnode_child_index(glaxnimate::model::DocumentNode* child) const
{
    return items.index_of(child);
}

void glaxnimate::model::Skin::on_parent_changed(model::DocumentNode*, model::DocumentNode* new_parent)
{
    auto sl = qobject_cast<SkinList*>(new_parent);
    skeleton_ = sl ? sl->skeleton() : nullptr;
}


glaxnimate::model::ShapeSkin::ShapeSkin(glaxnimate::model::Document* doc)
    : SkinItem(doc)
{
    connect(transform.get(), &Object::property_changed, this, &ShapeSkin::on_transform_matrix_changed);
}

void glaxnimate::model::ShapeSkin::on_paint(QPainter* p, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode, glaxnimate::model::Modifier*) const
{
    if ( auto attachment = this->attachment.get() )
        attachment->prepare_painter(p, t);
}

void glaxnimate::model::ShapeSkin::on_transform_matrix_changed()
{
    emit bounding_rect_changed();
    emit local_transform_matrix_changed(transform->transform_matrix());
    emit transform_matrix_changed(transform_matrix(time()));
}

QTransform glaxnimate::model::ShapeSkin::local_transform_matrix(model::FrameTime) const
{
    return transform->transform_matrix();
}

QIcon glaxnimate::model::ShapeSkin::tree_icon() const
{
    return QIcon::fromTheme("folder");
}

QString glaxnimate::model::ShapeSkin::type_name_human() const
{
    return tr("Shape");
}


