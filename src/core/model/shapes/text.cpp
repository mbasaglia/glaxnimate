#include "text.hpp"


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
        upscaled_raw();
    }

    void update_data()
    {
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

    QPainterPath path_for_glyph(quint32  glyph)
    {
        QPainterPath path = raw_scaled.pathForGlyph(glyph).simplified();
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

std::vector<model::Font::CharData> model::Font::line_data(const QString& line, CharDataCache& cache) const
{
    auto glyphs = d->raw.glyphIndexesForString(line);
    auto advances = d->raw.advancesForGlyphIndexes(glyphs, QRawFont::KernedAdvances);
    std::vector<model::Font::CharData> data;
    data.reserve(glyphs.size());

    for ( int i = 0; i < glyphs.size(); i++ )
    {
        auto it = cache.find(glyphs[i]);
        QPainterPath path;

        if ( it == cache.end() )
        {
            path = d->path_for_glyph(glyphs[i]);
            cache.emplace(glyphs[i], path);
        }
        else
        {
            path = it->second;
        }

        data.push_back({
            glyphs[i],
            advances[i],
            path
        });
    }

    return data;
}

qreal model::Font::line_spacing() const
{
    return d->metrics.lineSpacing();
}

QStringList model::Font::families() const
{
    return d->database.families();
}

QList<int> model::Font::standard_sizes() const
{
    return QFontDatabase::standardSizes();
}




void model::TextShape::add_shapes(model::FrameTime t, math::bezier::MultiBezier& bez) const
{
    bez.append(to_painter_path(t));
}

QPainterPath model::TextShape::to_painter_path(model::FrameTime t) const
{
    QPainterPath p;

    auto lines = text.get().split('\n');
    QPointF baseline = position.get_at(t);
    Font::CharDataCache cache;

    for ( const auto& line : lines )
    {
        QPointF start = baseline;

        for ( const auto& data : font->line_data(line, cache) )
        {
            p += data.path.translated(start);
            start += data.advance;
        }

        baseline.setY(baseline.y() + font->line_spacing());
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
