#include "repeater.hpp"

#include <QPainter>

GLAXNIMATE_OBJECT_IMPL(model::Repeater)

QIcon model::Repeater::static_tree_icon()
{
    return QIcon::fromTheme("table");
}

QString model::Repeater::static_type_name_human()
{
    return tr("Repeater");
}


math::bezier::MultiBezier model::Repeater::process(FrameTime t, const math::bezier::MultiBezier& mbez) const
{
    QTransform matrix = transform->transform_matrix(t);
    math::bezier::MultiBezier out;
    math::bezier::MultiBezier copy = mbez;
    for ( int i = 0; i < copies.get_at(t); i++ )
    {
        out.append(copy);
        copy.transform(matrix);
    }
    return out;

}

bool model::Repeater::process_collected() const
{
    return true;
}

void model::Repeater::on_paint(QPainter* painter, model::FrameTime t, model::VisualNode::PaintMode mode) const
{
    QTransform matrix = transform->transform_matrix(t);
    auto alpha_s = start_opacity.get_at(t);
    auto alpha_e = end_opacity.get_at(t);
    int n_copies = copies.get_at(t);

    for ( int i = 0; i < n_copies; i++ )
    {
        float alpha_lerp = float(i) / (n_copies == 1 ? 1 : n_copies - 1);
        auto alpha = math::lerp(alpha_s, alpha_e, alpha_lerp);
        painter->setOpacity(alpha * painter->opacity());

        for ( auto sib : affected() )
        {
            if ( sib->visible.get() )
                sib->paint(painter, t, mode);
        }

        painter->setTransform(matrix, true);
    }
}
