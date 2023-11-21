/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "text.hpp"

#include <QTextLayout>
#include <QFontInfo>
#include <QMetaEnum>

#include "group.hpp"
#include "path.hpp"
#include "command/undo_macro_guard.hpp"
#include "model/assets/assets.hpp"
#include "model/custom_font.hpp"
#include "math/bezier/bezier_length.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Font)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::TextShape)


class glaxnimate::model::Font::Private
{
public:
    QStringList styles;
    QFont query;
    QRawFont raw;
    QRawFont raw_scaled;
    QFontMetricsF metrics;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QFontDatabase database;
#endif

    Private() :
        raw(QRawFont::fromFont(query)),
        metrics(query)
    {
#ifdef Q_OS_ANDROID
        query.setPointSizeF(32);
#endif
//         query.setKerning(false);
        upscaled_raw();
    }

    void update_data()
    {
        // disable kerning because QRawFont doesn't handle kerning properly
//         query.setKerning(false);
        raw = QRawFont::fromFont(query);

        // Dynamic fonts might have weird names
        if ( !raw.familyName().startsWith(query.family()) )
        {
            QString family = query.family();
            QFont new_query = query;
            new_query.setFamily(family + ' ' + query.styleName());
            auto new_raw = QRawFont::fromFont(new_query);
            if ( new_raw.familyName().startsWith(family) )
            {
                query = new_query;
                raw = new_raw;
            }
        }

        metrics = QFontMetricsF(query);
        upscaled_raw();
    }

    static const QStringList& default_styles()
    {
        static QStringList styles;
        if ( styles.empty() )
        {
            auto meta = QMetaEnum::fromType<QFont::Weight>();
            for ( int i = 0; i < meta.keyCount(); i++ )
            {
                QString key = meta.key(i);
                for ( const char* style : {"", " Italic", " Oblique"} )
                {
                    styles.push_back(key + style);
                }
            }
        }

        return styles;
    }

    void refresh_styles(Font* parent)
    {
        if ( !raw.familyName().startsWith(query.family()) )
        {
            styles = default_styles();
        }
        else
        {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            styles = database.styles(parent->family.get());
#else
            styles = QFontDatabase::styles(parent->family.get());
#endif
            if ( !parent->valid_style(parent->style.get()) && !styles.empty() )
                parent->style.set(styles[0]);
        }
    }

    // QRawFont::pathForGlyph doesn't work well, so we work around it
    void upscaled_raw()
    {
        QFont font =  query;
#ifndef Q_OS_ANDROID
        font.setPointSizeF(qMin(4000., font.pointSizeF() * 1000));
#endif
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
            QPointF p = QPointF(element) * mult;
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

glaxnimate::model::Font::Font(glaxnimate::model::Document* doc)
    : Object(doc), d(std::make_unique<Private>())
{
    family.set(d->raw.familyName());
    style.set(d->raw.styleName());
    size.set(d->query.pointSize());
    d->refresh_styles(this);
    on_transfer(doc);
}

glaxnimate::model::Font::~Font() = default;

void glaxnimate::model::Font::refresh_data ( bool update_styles )
{
    d->query = CustomFontDatabase::instance().font(family.get(), style.get(), size.get());
    d->update_data();
    if ( update_styles )
        d->refresh_styles(this);
    Q_EMIT font_changed();
}

void glaxnimate::model::Font::on_font_changed()
{
    refresh_data(false);
}

void glaxnimate::model::Font::on_transfer ( model::Document* doc )
{
    if ( document() )
        disconnect(document()->assets()->fonts.get(), nullptr, this, nullptr);

    if ( doc )
    {
        connect(doc->assets()->fonts.get(), &FontList::font_added, this, [this]{
            refresh_data(true);
            document()->graphics_invalidated();
        });
    }
}


void glaxnimate::model::Font::on_family_changed()
{
    refresh_data(true);
}

bool glaxnimate::model::Font::valid_style(const QString& style)
{
    return d->styles.contains(style);
}

const QFont & glaxnimate::model::Font::query() const
{
    return d->query;
}

const QRawFont & glaxnimate::model::Font::raw_font() const
{
    return d->raw;
}

QStringList glaxnimate::model::Font::styles() const
{
    return d->styles;
}

const QFontMetricsF & glaxnimate::model::Font::metrics() const
{
    return d->metrics;
}

QString glaxnimate::model::Font::type_name_human() const
{
    return tr("Font");
}

QPainterPath glaxnimate::model::Font::path_for_glyph(quint32 glyph, glaxnimate::model::Font::CharDataCache& cache, bool fix_paint) const
{
    auto it = cache.find(glyph);

    if ( it != cache.end() )
        return it->second;

    QPainterPath path = d->path_for_glyph(glyph, fix_paint);
    cache.emplace(glyph, path);
    return path;
}

void glaxnimate::model::Font::from_qfont(const QFont& f)
{
    command::UndoMacroGuard g(tr("Change Font"), document());
    QFontInfo finfo(f);
    family.set_undoable(finfo.family());
    style.set_undoable(finfo.styleName());
    size.set_undoable(f.pointSizeF());
}



glaxnimate::model::Font::ParagraphData glaxnimate::model::Font::layout(const QString& text) const
{
    glaxnimate::model::Font::ParagraphData para_data;

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

qreal glaxnimate::model::Font::line_spacing() const
{
    // for some reason QTextLayout ignores leading()
    return line_spacing_unscaled() * line_height.get();
}

qreal glaxnimate::model::Font::line_spacing_unscaled() const
{
    // for some reason QTextLayout ignores leading()
    return d->metrics.ascent() + d->metrics.descent();
}


QStringList glaxnimate::model::Font::families() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return d->database.families();
#else
    return QFontDatabase::families();
#endif
}

QList<int> glaxnimate::model::Font::standard_sizes() const
{
    auto list = QFontDatabase::standardSizes();
    int actual = d->query.pointSize();
    auto it = std::upper_bound(list.begin(), list.end(), actual);
    if ( it == list.begin() || *(it-1) != actual )
        list.insert(it, actual);
    return list;
}

glaxnimate::model::TextShape::TextShape(glaxnimate::model::Document* document)
    : ShapeElement(document)
{
    connect(font.get(), &Font::font_changed, this, &TextShape::on_font_changed);
}

void glaxnimate::model::TextShape::on_text_changed()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    shape_cache.clear();
#else
    shape_cache = QPainterPath();
#endif
    propagate_bounding_rect_changed();
}

void glaxnimate::model::TextShape::on_font_changed()
{
    cache.clear();
    on_text_changed();
}

const QPainterPath & glaxnimate::model::TextShape::untranslated_path(FrameTime t) const
{
    if ( shape_cache.isEmpty() )
    {
        if ( path.get() )
        {
            QString txt = text.get();
            txt.replace('\n', ' ');
            auto bezier = path->shapes(t);
            const int length_steps = 5;

            math::bezier::LengthData length_data(bezier, length_steps);
            for ( const auto& line : font->layout(txt) )
            {
                for ( const auto& glyph : line.glyphs )
                {
                    qreal x = path_offset.get_at(t) + glyph.position.x();
                    if ( x > length_data.length() || x < 0 )
                        continue;

                    auto glyph_shape = font->path_for_glyph(glyph.glyph, cache, true);
                    auto glyph_rect = glyph_shape.boundingRect();

                    auto start1 = length_data.at_length(x);
                    auto start2 = start1.descend();
                    auto start_p = bezier.beziers()[start1.index].split_segment_point(start2.index, start2.ratio);

                    auto end1 = length_data.at_length(x + glyph_rect.width());
                    auto end2 = end1.descend();
                    auto end_p = bezier.beziers()[end1.index].split_segment_point(end2.index, end2.ratio);

                    QTransform mat;
                    mat.translate(start_p.pos.x(), start_p.pos.y());
                    mat.rotate(qRadiansToDegrees(math::atan2(end_p.pos.y() - start_p.pos.y(), end_p.pos.x() - start_p.pos.x())));
                    shape_cache += mat.map(glyph_shape);
                }
            }
        }
        else
        {
            for ( const auto& line : font->layout(text.get()) )
                for ( const auto& glyph : line.glyphs )
                    shape_cache += font->path_for_glyph(glyph.glyph, cache, true).translated(glyph.position);
        }
    }

    return shape_cache;
}


void glaxnimate::model::TextShape::add_shapes(glaxnimate::model::FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const
{
    if ( !transform.isIdentity() )
    {
        auto mb = math::bezier::MultiBezier::from_painter_path(shape_data(t));
        mb.transform(transform);
        bez.append(mb);
    }
    else
    {
        bez.append(shape_data(t));
    }
}

QPainterPath glaxnimate::model::TextShape::to_painter_path_impl(glaxnimate::model::FrameTime) const
{
    return {};
}

QPainterPath glaxnimate::model::TextShape::shape_data(FrameTime t) const
{
    // Ignore position if we have a path, it can still be moved from the group
    if ( path.get() )
        return untranslated_path(t);
    QPointF pos = position.get_at(t);
    return untranslated_path(t).translated(pos);
}

QIcon glaxnimate::model::TextShape::tree_icon() const
{
    return QIcon::fromTheme("font");
}

QRectF glaxnimate::model::TextShape::local_bounding_rect(glaxnimate::model::FrameTime t) const
{
    return shape_data(t).boundingRect();
}

QString glaxnimate::model::TextShape::type_name_human() const
{
    return tr("Text");
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::TextShape::to_path() const
{
    auto group = std::make_unique<glaxnimate::model::Group>(document());
    group->name.set(name.get());
    group->group_color.set(group_color.get());
    group->visible.set(visible.get());

    Font::CharDataCache local_cache;

    for ( const auto& line : font->layout(text.get()) )
    {
        auto line_group = std::make_unique<glaxnimate::model::Group>(document());
        line_group->name.set(line.text);

        for ( const auto& glyph : line.glyphs )
        {
            QPainterPath p = font->path_for_glyph(glyph.glyph, local_cache, false).translated(glyph.position);
            math::bezier::MultiBezier bez;
            bez.append(p);

            if ( bez.beziers().size() == 1 )
            {
                auto path = std::make_unique<glaxnimate::model::Path>(document());
                path->shape.set(bez.beziers()[0]);
                line_group->shapes.insert(std::move(path), 0);
            }
            else if ( bez.beziers().size() > 1 )
            {
                auto glyph_group = std::make_unique<glaxnimate::model::Group>(document());
                for ( const auto& sub : bez.beziers() )
                {
                    auto path = std::make_unique<glaxnimate::model::Path>(document());
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

QPointF glaxnimate::model::TextShape::offset_to_next_character() const
{
    auto layout = font->layout(text.get());
    if ( layout.empty() )
        return {};
    return layout.back().advance;
}

std::vector<glaxnimate::model::DocumentNode *> glaxnimate::model::TextShape::valid_paths() const
{
    std::vector<glaxnimate::model::DocumentNode *> shapes;
    shapes.push_back(nullptr);

    for ( const auto& sib : *owner() )
        if ( sib.get() != this )
            shapes.push_back(sib.get());

    return shapes;
}

bool glaxnimate::model::TextShape::is_valid_path(glaxnimate::model::DocumentNode* node) const
{
    if ( node == nullptr )
        return true;

    if ( node == this )
        return false;

    if ( auto shape = node->cast<glaxnimate::model::ShapeElement>() )
        return shape->owner_composition() == owner_composition();

    return false;
}

void glaxnimate::model::TextShape::path_changed(glaxnimate::model::ShapeElement* new_path, glaxnimate::model::ShapeElement* old_path)
{
    on_text_changed();

    if ( old_path )
        disconnect(old_path, nullptr, this, nullptr);

    if ( new_path )
    {
        connect(new_path, &Object::visual_property_changed, this, &TextShape::on_text_changed);
        connect(new_path, &VisualNode::bounding_rect_changed, this, &TextShape::on_text_changed);
    }
}

