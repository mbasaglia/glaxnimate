#include "skin.hpp"
#include "skeleton_p.hpp"
#include "model/document.hpp"


GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Skin)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::ImageSkin)

void glaxnimate::model::SkinItemBase::on_parent_changed(model::DocumentNode*, model::DocumentNode* new_parent)
{
    skin_ = qobject_cast<Skin*>(new_parent);
}

glaxnimate::model::Skin * glaxnimate::model::SkinItemBase::skin() const
{
    return skin_;
}

glaxnimate::model::Skeleton * glaxnimate::model::SkinItemBase::skeleton() const
{
    return skin_ ? skin_->skeleton() : nullptr;
}

glaxnimate::model::VisualNode * glaxnimate::model::SkinItemBase::docnode_group_parent() const
{
    return slot.get();
}

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::SkinItemBase::valid_slots() const
{
    auto skel = skeleton();
    if ( !skel )
        return {};
    return std::vector<DocumentNode*>(skel->d->skin_slots.begin(), skel->d->skin_slots.end());
}

bool glaxnimate::model::SkinItemBase::is_valid_slot(glaxnimate::model::DocumentNode* node) const
{
    if ( !node )
        return true;
    auto skel = skeleton();
    if ( !skel )
        return false;
    return skel->d->skin_slots.count(static_cast<SkinSlot*>(node));
}

QString glaxnimate::model::SkinItemBase::object_name() const
{
    QString base = VisualNode::object_name();
    if ( slot.get() )
        return slot->object_name() + " / " + base;
    return base;
}

void glaxnimate::model::SkinItemBase::on_slot_changed(glaxnimate::model::SkinSlot*, glaxnimate::model::SkinSlot*)
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


glaxnimate::model::ImageSkin::ImageSkin(glaxnimate::model::Document* doc)
    : Ctor(doc)
{
    connect(transform.get(), &Object::property_changed, this, &ImageSkin::on_transform_matrix_changed);
}


bool glaxnimate::model::ImageSkin::is_valid_image(glaxnimate::model::DocumentNode* node) const
{
    return document()->assets()->images->values.is_valid_reference_value(node, false);
}

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::ImageSkin::valid_images() const
{
    return document()->assets()->images->values.valid_reference_values(false);
}

QRectF glaxnimate::model::ImageSkin::local_bounding_rect(glaxnimate::model::FrameTime) const
{
    if ( !image.get() )
        return {};
    return QRectF(0, 0, image->width.get(), image->height.get());
}

void glaxnimate::model::ImageSkin::on_paint(QPainter* p, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode, glaxnimate::model::Modifier*) const
{
    if ( auto slot = this->slot.get() )
        slot->prepare_painter(p, t);

    if ( image.get() )
        image->paint(p);
}

void glaxnimate::model::ImageSkin::on_transform_matrix_changed()
{
    emit bounding_rect_changed();
    emit local_transform_matrix_changed(transform->transform_matrix());
    emit transform_matrix_changed(transform_matrix(time()));
}

void glaxnimate::model::ImageSkin::on_image_changed(glaxnimate::model::Bitmap* new_use, glaxnimate::model::Bitmap* old_use)
{
    if ( old_use )
    {
        disconnect(old_use, &Bitmap::loaded, this, &ImageSkin::on_update_image);
    }

    if ( new_use )
    {
        connect(new_use, &Bitmap::loaded, this, &ImageSkin::on_update_image);
    }
}

void glaxnimate::model::ImageSkin::on_update_image()
{
    emit property_changed(&image, {});
}

QTransform glaxnimate::model::ImageSkin::local_transform_matrix(model::FrameTime t) const
{
    return transform->transform_matrix(local_bounding_rect(t).center());
}

QIcon glaxnimate::model::ImageSkin::tree_icon() const
{
    return QIcon::fromTheme("x-shape-image");
}

QString glaxnimate::model::ImageSkin::type_name_human() const
{
    return tr("Image");
}


