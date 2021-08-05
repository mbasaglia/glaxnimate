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
