#include "skeleton_p.hpp"
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

void glaxnimate::model::Skeleton::on_paint(QPainter* painter, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode mode, model::Modifier* modifier) const
{
    if ( skin.get() )
    {
        std::map<SkinSlot*, std::vector<SkinItemBase*>> items;
        for ( const auto& item : skin->items )
            items[item->slot.get()].push_back(item.get());

        /// \todo draw order instead of `d->skin_slots`
        std::vector<SkinSlot*> skin_slots(d->skin_slots.begin(), d->skin_slots.end());
        std::sort(skin_slots.begin(), skin_slots.end(), [](const SkinSlot* a, const SkinSlot* b){ return a->draw_order.get() < b->draw_order.get(); });
        for ( const auto& slot : skin_slots )
            for ( const auto& item : items[slot] )
                item->paint(painter, t, mode, modifier);
    }

    for ( const auto& ch : bones->values )
        ch->paint(painter, t, mode, modifier);
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




