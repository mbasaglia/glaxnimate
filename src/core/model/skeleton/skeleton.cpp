#include "skeleton_p.hpp"

#include <QPainter>

#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Skeleton)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::BoneList)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::SkinList)


glaxnimate::model::Skeleton::Skeleton(model::Document* document)
    : ShapeElement(document), d(std::make_unique<Private>())
{
    bones->skeleton_ = this;
    skins->skeleton_ = this;
}

glaxnimate::model::Skeleton::~Skeleton()
{
}

QRectF glaxnimate::model::Skeleton::local_bounding_rect(glaxnimate::model::FrameTime t) const
{
    return range_bounding_rect(t, bones->values.begin(), bones->values.end());
}

QIcon glaxnimate::model::Skeleton::tree_icon() const
{
    return QIcon::fromTheme("bone");
}

glaxnimate::model::Bone * glaxnimate::model::Skeleton::add_bone()
{
    auto child = std::make_unique<Bone>(document());
    auto raw = child.get();
    push_command(new command::AddObject<BoneItem>(&bones->values, std::move(child), bones->values.size()));
    return raw;
}

QPainterPath glaxnimate::model::Skeleton::to_painter_path(glaxnimate::model::FrameTime) const
{
    return {};
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::Skeleton::to_path() const
{
    return clone_covariant();
}

void glaxnimate::model::Skeleton::add_shapes(FrameTime, math::bezier::MultiBezier&, const QTransform&) const
{
}

void glaxnimate::model::Skeleton::paint(QPainter* painter, FrameTime time, PaintMode mode, glaxnimate::model::Modifier* modifier) const
{
    if ( !visible.get() )
        return;

    if ( skin.get() )
    {
        painter->save();
        painter->setTransform(transform_matrix(time).inverted(), true);

        std::map<SkinSlot*, std::vector<SkinItem*>> items;
        for ( const auto& item : skin->items )
        {
            auto slot = item->slot();
            if ( slot && slot->attachment.get() == item->attachment.get() )
                items[slot].push_back(item.get());
        }

        std::vector<SkinSlot*> skin_slots(d->skin_slots.begin(), d->skin_slots.end());
        std::sort(skin_slots.begin(), skin_slots.end(), [](const SkinSlot* a, const SkinSlot* b){ return a->draw_order.get() < b->draw_order.get(); });
        for ( const auto& slot : skin_slots )
            for ( const auto& item : items[slot] )
                item->paint(painter, time, mode, modifier);

        painter->restore();
    }

    painter->save();
    painter->setTransform(group_transform_matrix(time), true);

    for ( const auto& ch : bones->values )
        ch->paint(painter, time, mode, modifier);

    painter->restore();
}

glaxnimate::model::Skin * glaxnimate::model::Skeleton::add_skin()
{
    auto child = std::make_unique<Skin>(document());
    auto raw = child.get();
    push_command(new command::AddObject(&skins->values, std::move(child), skins->values.size()));
    return raw;
}

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::Skeleton::valid_skins() const
{
    std::vector<DocumentNode *> valid;
    for ( const auto& skin : skins->values )
        valid.push_back(skin.get());
    return valid;
}

bool glaxnimate::model::Skeleton::is_valid_skin(glaxnimate::model::DocumentNode* node) const
{
    if ( !node )
        return true;
    return skins->values.index_of(node) != -1;
}

void glaxnimate::model::Skeleton::on_skin_changed(glaxnimate::model::Skin*, glaxnimate::model::Skin*)
{
    emit bounding_rect_changed();
}


glaxnimate::model::DocumentNode * glaxnimate::model::Skeleton::docnode_child(int i) const
{
    switch ( i )
    {
        case 0: return const_cast<BoneList*>(bones.get());
        case 1: return const_cast<SkinList*>(skins.get());
        default: return nullptr;
    }
}

int glaxnimate::model::Skeleton::docnode_child_count() const
{
    return 2;
}

int glaxnimate::model::Skeleton::docnode_child_index(glaxnimate::model::DocumentNode* dn) const
{
    if ( dn == bones.get() )
        return 0;
    if ( dn == skins.get() )
        return 1;
    return -1;
}


