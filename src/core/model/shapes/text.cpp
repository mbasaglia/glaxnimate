#include "text.hpp"

GLAXNIMATE_OBJECT_IMPL(model::TextShape)

QRawFont model::TextShape::font() const
{
    return QRawFont::fromFont(QFont("sans", size.get()));
}

void model::TextShape::add_shapes(model::FrameTime t, math::bezier::MultiBezier& bez) const
{
    bez.append(to_painter_path(t));
}

QPainterPath model::TextShape::to_painter_path(model::FrameTime t) const
{
    QPainterPath p;
    QRawFont font = this->font();
    auto glyhs = font.glyphIndexesForString(text.get());
    auto advances = font.advancesForGlyphIndexes(glyhs, QRawFont::KernedAdvances);
    QPointF start = position.get_at(t);
    for ( int i = 0; i < glyhs.size(); i++ )
    {
        QPainterPath glyph_path = font.pathForGlyph(glyhs[i]);
        glyph_path.translate(start);
        start += advances[i];
        p += glyph_path;
    }
    return p;
}

QIcon model::TextShape::docnode_icon() const
{
    return QIcon::fromTheme("font");
}

QRectF model::TextShape::local_bounding_rect(model::FrameTime t) const
{
    return to_painter_path(t).boundingRect();
}
