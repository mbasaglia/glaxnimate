#include "text.hpp"

#include <QTextLayout>
#include <QFontInfo>

#include "group.hpp"
#include "path.hpp"
#include "command/undo_macro_guard.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Font)
GLAXNIMATE_OBJECT_IMPL(model::TextShape)

class model::Font::Private
{
public:
    QStringList styles;
    QFont query;
    QRawFont raw;
    QRawFont raw_scaled;
    QFontMetricsF metrics;
    QFontDatabase database;

    Private() :
        raw(QRawFont::fromFont(query)),
        metrics(query)
    {
//         query.setKerning(false);
        upscaled_raw();
    }

    void update_data()
    {
        // disable kerning because QRawFont doesn't handle kerning properly
//         query.setKerning(false);

        raw = QRawFont::fromFont(query);
        metrics = QFontMetricsF(query);
        upscaled_raw();
    }

    void refresh_styles(Font* parent)
    {
        styles = database.styles(parent->family.get());
        if ( !parent->valid_style(parent->style.get()) && !styles.empty() )
            parent->style.set(styles[0]);
    }

    // QRawFont::pathForGlyph doesn't work well, so we work around it
    void upscaled_raw()
    {
        QFont font =  query;
        font.setPointSizeF(font.pointSizeF() * 1000);
        raw_scaled = QRawFont::fromFont(font);
    }

    QPainterPath path_for_glyph(quint32  glyph, bool fix_paint)
    {

        QPainterPath path = raw_scaled.pathForGlyph(glyph);

        if ( fix_paint )
            path = path.simplified();

        if ( raw_scaled.pixelSize() == 0 )
            return path;

        QPainterPath dest;
        qreal mult = raw.pixelSize() / raw_scaled.pixelSize();

        std::array<QPointF, 3> data;
        int data_i = 0;
        for ( int i = 0; i < path.elementCount(); i++ )
        {
            auto element = path.elementAt(i);
            QPointF p = element * mult;
            switch ( element.type )
            {
                case QPainterPath::MoveToElement:
                    dest.moveTo(p);
                    break;
                case QPainterPath::LineToElement:
                    dest.lineTo(p);
                    break;
                case QPainterPath::CurveToElement:
                    data_i = 0;
                    data[0] = p;
                    break;
                case QPainterPath::CurveToDataElement:
                    ++data_i;
                    data[data_i] = p;
                    if ( data_i == 2 )
                    {
                        dest.cubicTo(data[0], data[1], data[2]);
                        data_i = -1;
                    }
                    break;
            }
        }

        return dest;
    }
};

model::Font::Font(model::Document* doc)
    : Object(doc), d(std::make_unique<Private>())
{
    family.set(d->raw.familyName());
    style.set(d->raw.styleName());
    size.set(d->query.pointSize());
    d->refresh_styles(this);
}

model::Font::~Font() = default;

void model::Font::on_font_changed()
{
    d->query = QFont(family.get(), size.get());
    d->query.setStyleName(style.get());
//     query_.setHintingPreference(QFont::PreferFullHinting);
//     query_.setStyleStrategy(QFont::StyleStrategy(QFont::ForceOutline|QFont::PreferQuality));
    d->update_data();
    emit font_changed();

}

void model::Font::on_family_changed()
{
    d->refresh_styles(this);
    on_font_changed();
}

bool model::Font::valid_style(const QString& style)
{
    return d->styles.contains(style);
}

const QFont & model::Font::query() const
{
    return d->query;
}

const QRawFont & model::Font::raw_font() const
{
    return d->raw;
}

QStringList model::Font::styles() const
{
    return d->styles;
}

const QFontMetricsF & model::Font::metrics() const
{
    return d->metrics;
}

QString model::Font::type_name_human() const
{
    return tr("Font");
}

QPainterPath model::Font::path_for_glyph(quint32 glyph, model::Font::CharDataCache& cache, bool fix_paint) const
{
    auto it = cache.find(glyph);

    if ( it != cache.end() )
        return it->second;

    QPainterPath path = d->path_for_glyph(glyph, fix_paint);
    cache.emplace(glyph, path);
    return path;
}

void model::Font::from_qfont(const QFont& f)
{
    command::UndoMacroGuard g(tr("Change Font"), document());
    QFontInfo finfo(f);
    family.set_undoable(finfo.family());
    style.set_undoable(finfo.styleName());
    size.set_undoable(f.pointSizeF());
}



model::Font::ParagraphData model::Font::layout(const QString& text) const
{
    model::Font::ParagraphData para_data;

    auto lines = text.split('\n');
    QTextLayout layout(text, d->query, nullptr);

    QTextOption option;
    option.setUseDesignMetrics(true);
    layout.setTextOption(option);
    layout.beginLayout();
    for ( const auto& line_size : lines )
    {
        QTextLine line = layout.createLine();
        if ( !line.isValid() )
            break;
        line.setNumColumns(line_size.size());
        line.setLeadingIncluded(true);
    }
    layout.endLayout();

    qreal line_y = 0;
    qreal yoff = -d->metrics.ascent();

    for ( int ln = 0; ln < layout.lineCount(); ln++ )
    {
        QTextLine line = layout.lineAt(ln);

        auto& line_data = para_data.emplace_back();
        line_data.baseline = QPointF(0, line_y);
        line_data.bounds = line.rect();
        line_data.text = lines[ln];

        QPointF baseline(0, line_y + yoff);
        for ( const auto& run : line.glyphRuns() )
        {
            auto glyphs = run.glyphIndexes();
            line_data.glyphs.reserve(line_data.glyphs.size() + glyphs.size());
            auto positions = run.positions();
            for ( int i = 0; i < glyphs.size(); i++ )
            {
                line_data.glyphs.push_back({
                    glyphs[i],
                    positions[i] + baseline
                });
            }
        }

        line_data.advance = QPointF(line.cursorToX(lines[ln].size()), 0);

        line_y += line_spacing();
    }

    // QRawFont way: for some reason it ignores KernedAdvances
    /*
    qreal line_y = 0;
    for ( const auto& line : text.split('\n') )
    {
        auto glyphs = d->raw.glyphIndexesForString(line);
        auto advances = d->raw.advancesForGlyphIndexes(glyphs, QRawFont::UseDesignMetrics|QRawFont::KernedAdvances);

        auto& line_data = para_data.emplace_back();
        line_data.glyphs.reserve(glyphs.size());
        line_data.text = line;

        line_data.baseline = line_data.advance = QPointF(0, line_y);
        for ( int i = 0; i < glyphs.size(); i++ )
        {
            line_data.glyphs.push_back({
                glyphs[i],
                line_data.advance,
            });
            line_data.advance += advances[i];
        }

        line_y += line_spacing();
    }
    */
    return para_data;
}

qreal model::Font::line_spacing() const
{
    // for some reason QTextLayout ignores leading()
    return line_spacing_unscaled() * line_height.get();
}

qreal model::Font::line_spacing_unscaled() const
{
    // for some reason QTextLayout ignores leading()
    return d->metrics.ascent() + d->metrics.descent();
}


QStringList model::Font::families() const
{
    return d->database.families();
}

QList<int> model::Font::standard_sizes() const
{
    auto list = QFontDatabase::standardSizes();
    int actual = d->query.pointSize();
    auto it = std::upper_bound(list.begin(), list.end(), actual);
    if ( it == list.begin() || *(it-1) != actual )
        list.insert(it, actual);
    return list;
}

model::TextShape::TextShape(model::Document* document)
    : ShapeElement(document)
{
    connect(font.get(), &Font::font_changed, this, &TextShape::on_font_changed);
}

void model::TextShape::on_text_changed()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    shape_cache.clear();
#else
    shape_cache = QPainterPath();
#endif
}

void model::TextShape::on_font_changed()
{
    on_text_changed();
    cache.clear();
}

const QPainterPath & model::TextShape::untranslated_path() const
{
    if ( shape_cache.isEmpty() )
    {
        for ( const auto& line : font->layout(text.get()) )
            for ( const auto& glyph : line.glyphs )
                shape_cache += font->path_for_glyph(glyph.glyph, cache, true).translated(glyph.position);
    }

    return shape_cache;
}


void model::TextShape::add_shapes(model::FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const
{
    if ( !transform.isIdentity() )
    {
        auto mb = math::bezier::MultiBezier::from_painter_path(to_painter_path(t));
        mb.transform(transform);
        bez.append(mb);
    }
    else
    {
        bez.append(to_painter_path(t));
    }
}

QPainterPath model::TextShape::to_painter_path(model::FrameTime t) const
{
    QPointF pos = position.get_at(t);
    return untranslated_path().translated(pos);
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

std::unique_ptr<model::ShapeElement> model::TextShape::to_path() const
{
    auto group = std::make_unique<model::Group>(document());
    group->name.set(name.get());
    group->group_color.set(group_color.get());
    group->visible.set(visible.get());

    for ( const auto& line : font->layout(text.get()) )
    {
        auto line_group = std::make_unique<model::Group>(document());
        line_group->name.set(line.text);

        for ( const auto& glyph : line.glyphs )
        {
            QPainterPath p = font->path_for_glyph(glyph.glyph, cache, false).translated(glyph.position);
            math::bezier::MultiBezier bez;
            bez.append(p);

            if ( bez.beziers().size() == 1 )
            {
                auto path = std::make_unique<model::Path>(document());
                path->shape.set(bez.beziers()[0]);
                line_group->shapes.insert(std::move(path), 0);
            }
            else if ( bez.beziers().size() > 1 )
            {
                auto glyph_group = std::make_unique<model::Group>(document());
                for ( const auto& sub : bez.beziers() )
                {
                    auto path = std::make_unique<model::Path>(document());
                    path->shape.set(sub);
                    glyph_group->shapes.insert(std::move(path), 0);
                }
                line_group->shapes.insert(std::move(glyph_group), 0);
            }
        }

        group->shapes.insert(std::move(line_group), 0);
    }

    group->set_time(time());
    if ( position.animated() )
    {
        for ( const auto& kf : position )
        {
            group->transform->position.set_keyframe(kf.time(), kf.get())->set_transition(kf.transition());
//             group->transform->anchor_point.set_keyframe(kf.time(), kf.get())->set_transition(kf.transition());
        }
    }

    group->transform->position.set(position.get());
//     group->transform->anchor_point.set(position.get());

    return group;
}

QPointF model::TextShape::offset_to_next_character() const
{
    auto layout = font->layout(text.get());
    if ( layout.empty() )
        return {};
    return layout.back().advance;
}
