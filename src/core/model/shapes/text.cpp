#include "text.hpp"


GLAXNIMATE_OBJECT_IMPL(model::Font)
GLAXNIMATE_OBJECT_IMPL(model::TextShape)

model::Font::Font(model::Document* doc)
    : Object(doc),
    raw_(QRawFont::fromFont(query_))
{
    family.set(raw_.familyName());
    style.set(raw_.styleName());
    size.set(query_.pointSize());
    refresh_styles();
}

void model::Font::on_font_changed()
{
    query_ = QFont(family.get(), size.get());
    query_.setStyleName(style.get());
    raw_ = QRawFont::fromFont(query_);
}

void model::Font::on_family_changed()
{
    refresh_styles();
    on_font_changed();
}

void model::Font::refresh_styles()
{
    styles_ = QFontDatabase().styles(family.get());
    if ( !valid_style(style.get()) && !styles_.empty() )
        style.set(styles_[0]);
}

bool model::Font::valid_style(const QString& style)
{
    return styles_.contains(style);
}

const QFont & model::Font::query() const
{
    return query_;
}

const QRawFont & model::Font::raw_font() const
{
    return raw_;
}

const QStringList & model::Font::styles() const
{
    return styles_;
}

QString model::Font::type_name_human() const
{
    return tr("Font");
}


void model::TextShape::add_shapes(model::FrameTime t, math::bezier::MultiBezier& bez) const
{
    bez.append(to_painter_path(t));
}

QPainterPath model::TextShape::to_painter_path(model::FrameTime t) const
{
    QPainterPath p;
    QRawFont font = this->font->raw_font();
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

QIcon model::TextShape::tree_icon() const
{
    return QIcon::fromTheme("font");
}

QRectF model::TextShape::local_bounding_rect(model::FrameTime t) const
{
    return to_painter_path(t).boundingRect();
}

QString model::TextShape::type_name_human() const
{
    return tr("Text");
}
