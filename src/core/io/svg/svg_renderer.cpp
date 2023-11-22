/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "svg_renderer.hpp"

#include "model/document.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/layer.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/shapes/rect.hpp"
#include "model/shapes/ellipse.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/text.hpp"
#include "model/shapes/repeater.hpp"
#include "model/animation/join_animatables.hpp"
#include "model/custom_font.hpp"
#include "math/math.hpp"

#include "detail.hpp"
#include "font_weight.hpp"
#include "io/utils.hpp"

using namespace glaxnimate::io::svg::detail;
using namespace glaxnimate;

class io::svg::SvgRenderer::Private
{
public:
    void collect_defs(model::Composition* comp)
    {
        if ( !at_start )
            return;

        fps = comp->fps.get();
        ip = comp->animation->first_frame.get();
        op = comp->animation->last_frame.get();
        if ( ip >= op )
            animated = NotAnimated;

        at_start = false;
        defs = element(svg, "defs");
        for ( const auto& color : comp->document()->assets()->colors->values )
            write_named_color(defs, color.get());
        for ( const auto& color : comp->document()->assets()->gradient_colors->values )
            write_gradient_colors(defs, color.get());
        for ( const auto& gradient : comp->document()->assets()->gradients->values )
            write_gradient(defs, gradient.get());

        auto view = element(svg, "sodipodi:namedview");
        view.setAttribute("inkscape:pagecheckerboard", "true");
        view.setAttribute("borderlayer", "true");
        view.setAttribute("bordercolor", "#666666");
        view.setAttribute("pagecolor", "#ffffff");
        view.setAttribute("inkscape:document-units", "px");

        add_fonts(comp->document());

        write_meta(comp);
    }

    void write_meta(model::Composition* comp)
    {
        auto rdf = element(element(svg, "metadata"), "rdf:RDF");
        auto work = element(rdf, "cc:Work");
        element(work, "dc:format").appendChild(dom.createTextNode("image/svg+xml"));
        QString dc_type = animated ? "MovingImage" : "StillImage";
        element(work, "dc:type").setAttribute("rdf:resource", "http://purl.org/dc/dcmitype/" + dc_type);
        element(work, "dc:title").appendChild(dom.createTextNode(comp->name.get()));
        auto document = comp->document();

        if ( document->info().empty() )
            return;

        if ( !document->info().author.isEmpty() )
            element(element(element(work, "dc:creator"), "cc:Agent"), "dc:title").appendChild(dom.createTextNode(document->info().author));

        if ( !document->info().description.isEmpty() )
            element(work, "dc:description").appendChild(dom.createTextNode(document->info().description));

        if ( !document->info().keywords.empty() )
        {
            auto bag = element(element(work, "dc:subject"), "rdf:Bag");
            for ( const auto& kw: document->info().keywords )
                element(bag, "rdf:li").appendChild(dom.createTextNode(kw));
        }
    }

    void add_fonts(model::Document* document)
    {
        if ( font_type == CssFontType::None )
            return;

        QString css;
        static QString font_face = R"(
@font-face {
    font-family: '%1';
    font-style: %2;
    font-weight: %3;
    src: url(%4);
}
)";

        for ( const auto & font : document->assets()->fonts->values )
        {
            auto custom = font->custom_font();
            if ( !custom.is_valid() )
                continue;

            QRawFont raw = custom.raw_font();
            auto type = qMin(suggested_type(font.get()), font_type);

            if ( type == CssFontType::Link )
            {
                auto link = element(svg, "link");
                link.setAttribute("xmlns", "http://www.w3.org/1999/xhtml");
                link.setAttribute("rel", "stylesheet");
                link.setAttribute("href", font->css_url.get());
                link.setAttribute("type", "text/css");
            }
            else if ( type == CssFontType::FontFace )
            {
                css += font_face
                    .arg(custom.family())
                    .arg(WeightConverter::convert(raw.weight(), WeightConverter::qt, WeightConverter::css))
                    .arg(raw.style() == QFont::StyleNormal ? 0 : 1)
                    .arg(font->source_url.get())
                ;
            }
            else if ( type == CssFontType::Embedded )
            {
                QString base64_encoded = font->data.get().toBase64(QByteArray::Base64UrlEncoding);
                QString format = model::CustomFontDatabase::font_data_format(font->data.get()) == model::FontFileFormat::OpenType ? "opentype" : "ttf";

                css += font_face
                    .arg(custom.family())
                    .arg(WeightConverter::convert(raw.weight(), WeightConverter::qt, WeightConverter::css))
                    .arg(raw.style() == QFont::StyleNormal ? 0 : 1)
                    .arg("data:application/x-font-" + format + ";charset=utf-8;base64," + base64_encoded)
                ;
            }
        }

        if ( !css.isEmpty() )
            element(svg, "style").appendChild(dom.createTextNode(css));
    }

    QDomElement element(QDomNode parent, const char* tag)
    {
        QDomElement e = dom.createElement(tag);
        parent.appendChild(e);
        return e;
    }

    void write_composition(QDomElement& parent, model::Composition* comp)
    {
        for ( const auto& lay : comp->shapes )
            write_shape(parent, lay.get(), false);
    }

    void write_visibility_attributes(QDomElement& parent, model::VisualNode* node)
    {
        if ( !node->visible.get() )
            parent.setAttribute("display", "none");
        if ( node->locked.get() )
            parent.setAttribute("sodipodi:insensitive", "true");
    }

    void write_shapes(QDomElement& parent, const model::ShapeListProperty& shapes, bool has_mask = false)
    {
        if ( shapes.empty() )
            return;

        auto it = shapes.begin();
        if ( has_mask )
            ++it;

        for ( ; it != shapes.end(); ++it )
            write_shape(parent, it->get(), false);
    }


    QString styler_to_css(model::Styler* styler)
    {
        if ( styler->use.get() )
            return "url(#" + non_uuid_ids_map[styler->use.get()] + ")";
        if ( styler->color.get().alpha() == 0 )
            return "transparent";
        return styler->color.get().name();
    }

    QDomElement write_styler_shapes(QDomElement& parent, model::Styler* styler, const Style::Map& style)
    {
        if ( styler->affected().size() == 1 )
        {
            write_shape_shape(parent, styler->affected()[0], style);
            write_visibility_attributes(parent, styler);
            parent.setAttribute("id", id(styler));
            return parent;
        }

        auto g = start_group(parent, styler);
        write_style(g, style);
        write_visibility_attributes(g, styler);
        g.setAttribute("id", id(styler));

        for ( model::ShapeElement* subshape : styler->affected() )
        {
            write_shape_shape(g, subshape, style);
        }

        return g;
    }

    QString unlerp_time(model::FrameTime time) const
    {
        return QString::number(math::unlerp(ip, op, time), 'f');
    }

    struct AnimationData
    {
        struct Attribute
        {
            QString attribute;
            QStringList values = {};
        };

        AnimationData(SvgRenderer::Private* parent, const std::vector<QString>& attrs, int n_keyframes,
                      qreal time_stretch, model::FrameTime time_start)
            : parent(parent), time_stretch(time_stretch), time_start(time_start)
        {
            attributes.reserve(attrs.size());
            for ( const auto& attr : attrs )
            {
                attributes.push_back({attr});
                attributes.back().values.reserve(n_keyframes);
            }
        }

        QString key_spline(const model::KeyframeTransition& trans)
        {
            return QString("%1 %2 %3 %4")
                .arg(trans.before().x(), 0, 'f')
                .arg(trans.before().y(), 0, 'f')
                .arg(trans.after().x(), 0, 'f')
                .arg(trans.after().y(), 0, 'f')
            ;
        }

        void add_values(const std::vector<QString>& vals)
        {
            for ( std::size_t i = 0; i != attributes.size(); i++ )
                attributes[i].values.push_back(vals[i]);
        }

        void add_keyframe(model::FrameTime time, const std::vector<QString>& vals,
                          const model::KeyframeTransition& trans)
        {
            if ( time < parent->ip || time > parent->op )
                return;

            if ( key_times.empty() && time > parent->ip )
            {
                key_times.push_back("0");
                key_splines.push_back("0 0 1 1");
                add_values(vals);
            }
            else if ( hold && last + 1 < time )
            {

                key_times.push_back(parent->unlerp_time(time - 1));
                key_splines.push_back("0 0 1 1");
                for ( std::size_t i = 0; i != attributes.size(); i++ )
                    attributes[i].values.push_back(attributes[i].values.back());

            }

            key_times.push_back(parent->unlerp_time(time));
            key_splines.push_back(key_spline(trans));

            for ( std::size_t i = 0; i != attributes.size(); i++ )
                attributes[i].values.push_back(vals[i]);

            last = time;
            hold = trans.hold();
        }

        void add_dom(
            QDomElement& element, const char* tag = "animate", const QString& type = {},
            const QString& path = {}, bool auto_orient = false
        )
        {
            if ( last < parent->op && path.isEmpty() )
            {
                key_times.push_back("1");
                for ( auto& attr : attributes )
                {
                    if ( !attr.values.empty() )
                        attr.values.push_back(attr.values.back());
                }
            }
            else
            {
                key_splines.pop_back();
            }

            QString key_times_str = key_times.join("; ");
            QString key_splines_str = key_splines.join("; ");
            for ( const auto& data : attributes )
            {
                QDomElement animation = parent->element(element, tag);
                animation.setAttribute("begin", parent->clock(time_start + time_stretch * parent->ip));
                animation.setAttribute("dur", parent->clock(time_start + time_stretch * parent->op-parent->ip));
                animation.setAttribute("attributeName", data.attribute);
                animation.setAttribute("calcMode", "spline");
                if ( !path.isEmpty() )
                {
                    animation.setAttribute("path", path);
                    if ( auto_orient )
                        animation.setAttribute("rotate", "auto");
                }
                animation.setAttribute("keyTimes", key_times_str);
                animation.setAttribute("keySplines", key_splines_str);
                animation.setAttribute("repeatCount", "indefinite");
                if ( !type.isEmpty() )
                    animation.setAttribute("type", type);
            }
        }

        SvgRenderer::Private* parent;
        std::vector<Attribute> attributes;
        QStringList key_times = {};
        QStringList key_splines = {};
        model::FrameTime last = 0;
        bool hold = false;
        qreal time_stretch = 1;
        model::FrameTime time_start = 0;
    };

    void write_property(
        QDomElement& element,
        model::AnimatableBase* property,
        const QString& attr
    )
    {
        element.setAttribute(attr, property->value().toString());

        if ( animated )
        {
            if ( property->keyframe_count() < 2 )
                return;

            auto keyframes = split_keyframes(property);

            AnimationData data(this, {attr}, keyframes.size(), time_stretch, time_start);

            for ( int i = 0; i < int(keyframes.size()); i++ )
            {
                auto kf = keyframes[i].get();
                data.add_keyframe(time_to_global(kf->time()), {kf->value().toString()}, kf->transition());
            }

            data.add_dom(element);
        }
    }

    qreal time_to_global(qreal time)
    {
        for ( auto it = timing.rbegin(), end = timing.rend(); it != end; ++it )
            time = (*it)->time_from_local(time);
        return time;
    }

    template<class Callback>
    void write_properties(
        QDomElement& element,
        std::vector<const model::AnimatableBase*> properties,
        const std::vector<QString>& attrs,
        const Callback& callback
    )
    {
        auto jflags = animated == NotAnimated ? model::JoinAnimatables::NoKeyframes : model::JoinAnimatables::Normal;
        model::JoinedAnimatable j(std::move(properties), {}, jflags);

        {
            auto vals = callback(j.current_value());
            for ( std::size_t i = 0; i != attrs.size(); i++ )
                element.setAttribute(attrs[i], vals[i]);
        }

        if ( j.animated() && animated )
        {
            auto keys = split_keyframes(&j);
            AnimationData data(this, attrs, keys.size(), time_stretch, time_start);

            for ( const auto& kf : keys )
                data.add_keyframe(time_to_global(kf->time()), callback(j.value_at(kf->time())), kf->transition());

            data.add_dom(element);
        }
    }

    static std::vector<QString> callback_point(const std::vector<QVariant>& values)
    {
        return callback_point_result(values[0].toPointF());
    }

    static std::vector<QString> callback_point_result(const QPointF& c)
    {
        return std::vector<QString>{
            QString::number(c.x()),
            QString::number(c.y())
        };
    }

    void write_shape_rect(QDomElement& parent, model::Rect* rect, const Style::Map& style)
    {
        auto e = element(parent, "rect");
        write_style(e, style);
        write_properties(e, {&rect->position, &rect->size}, {"x", "y"},
            [](const std::vector<QVariant>& values){
                QPointF c = values[0].toPointF();
                QSizeF s = values[1].toSizeF();
                return std::vector<QString>{
                    QString::number(c.x() - s.width()/2),
                    QString::number(c.y() - s.height()/2)
                };
            }
        );
        write_properties(e, {&rect->size}, {"width", "height"},
            [](const std::vector<QVariant>& values){
                QSizeF s = values[0].toSizeF();
                return std::vector<QString>{
                    QString::number(s.width()),
                    QString::number(s.height())
                };
            }
        );
        write_property(e, &rect->rounded, "ry");
    }

    void write_shape_ellipse(QDomElement& parent, model::Ellipse* ellipse, const Style::Map& style)
    {
        auto e = element(parent, "ellipse");
        write_style(e, style);
        write_properties(e, {&ellipse->position}, {"cx", "cy"}, &Private::callback_point);
        write_properties(e, {&ellipse->size}, {"rx", "ry"},
            [](const std::vector<QVariant>& values){
                QSizeF s = values[0].toSizeF();
                return std::vector<QString>{
                    QString::number(s.width() / 2),
                    QString::number(s.height() / 2)
                };
            }
        );
    }

    void write_shape_star(QDomElement& parent, model::PolyStar* star, const Style::Map& style)
    {
        model::FrameTime time = star->time();

        auto e = write_bezier(parent, star, style);

        if ( star->outer_roundness.animated() || !qFuzzyIsNull(star->outer_roundness.get()) ||
             star->inner_roundness.animated() || !qFuzzyIsNull(star->inner_roundness.get()) )
            return;

        set_attribute(e, "sodipodi:type", "star");
        set_attribute(e, "inkscape:randomized", "0");
        // inkscape:rounded Works differently than lottie so we leave it as 0
        set_attribute(e, "inkscape:rounded", "0");
        int sides = star->points.get_at(time);
        set_attribute(e, "sodipodi:sides", sides);
        set_attribute(e, "inkscape:flatsided", star->type.get() == model::PolyStar::Polygon);
        QPointF c = star->position.get_at(time);
        set_attribute(e, "sodipodi:cx", c.x());
        set_attribute(e, "sodipodi:cy", c.y());
        set_attribute(e, "sodipodi:r1", star->outer_radius.get_at(time));
        set_attribute(e, "sodipodi:r2", star->inner_radius.get_at(time));
        qreal angle = math::deg2rad(star->angle.get_at(time) - 90);
        set_attribute(e, "sodipodi:arg1", angle);
        set_attribute(e, "sodipodi:arg2", angle + math::pi / sides);
    }

    void write_shape_text(QDomElement& parent, model::TextShape* text, Style::Map style)
    {
        QFontInfo font_info(text->font->query());

        // QFontInfo is broken, so we do something else
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        int weight = QFontDatabase::weight(font_info.family(), font_info.styleName());
        QFont::Style font_style = QFontDatabase::italic(font_info.family(), font_info.styleName()) ? QFont::StyleItalic : QFont::StyleNormal;
#else
        QFontDatabase db;
        int weight = db.weight(font_info.family(), font_info.styleName());
        QFont::Style font_style = db.italic(font_info.family(), font_info.styleName()) ? QFont::StyleItalic : QFont::StyleNormal;
#endif

        // Convert weight
        weight = WeightConverter::convert(weight, WeightConverter::qt, WeightConverter::css);

        style["font-family"] = font_info.family();
        style["font-size"] = QString("%1pt").arg(font_info.pointSizeF());
        style["line-height"] = QString("%1px").arg(text->font->line_spacing());
        style["font-weight"] = QString::number(weight);
        switch ( font_style )
        {
            case QFont::StyleNormal: style["font-style"] = "normal"; break;
            case QFont::StyleItalic: style["font-style"] = "italic"; break;
            case QFont::StyleOblique: style["font-style"] = "oblique"; break;
        }

        auto e = element(parent, "text");
        write_style(e, style);
        write_properties(e, {&text->position}, {"x", "y"}, &Private::callback_point);

        model::Font::CharDataCache cache;
        for ( const auto& line : text->font->layout(text->text.get()) )
        {
            auto tspan = element(e, "tspan");
            tspan.appendChild(dom.createTextNode(line.text));
            set_attribute(tspan, "sodipodi:role", "line");

            write_properties(tspan, {&text->position}, {"x", "y"}, [base=line.baseline](const std::vector<QVariant>& values){
                return callback_point_result(values[0].toPointF() + base);
            });
            tspan.setAttribute("xml:space", "preserve");
        }
    }

    void write_shape_shape(QDomElement& parent, model::ShapeElement* shape, const Style::Map& style)
    {
        if ( auto rect = qobject_cast<model::Rect*>(shape) )
        {
            write_shape_rect(parent, rect, style);
        }
        else if ( auto ellipse = qobject_cast<model::Ellipse*>(shape) )
        {
            write_shape_ellipse(parent, ellipse, style);
        }
        else if ( auto star = qobject_cast<model::PolyStar*>(shape) )
        {
            write_shape_star(parent, star, style);
        }
        else if ( auto text = shape->cast<model::TextShape>() )
        {
            write_shape_text(parent, text, style);
        }
        else if ( !qobject_cast<model::Styler*>(shape) )
        {
            write_bezier(parent, shape, style);
        }
    }

    void write_styler_attrs(QDomElement& element, model::Styler* styler, const QString& attr)
    {
        if ( styler->use.get() )
        {
            element.setAttribute(attr, "url(#" + non_uuid_ids_map[styler->use.get()] + ")");
            return;
        }

        write_property(element, &styler->color, attr);
        write_property(element, &styler->opacity, attr+"-opacity");
    }
    void write_image(model::Image* img, QDomElement& parent)
    {
        if ( img->image.get() )
        {
            auto e = element(parent, "image");
            set_attribute(e, "x", 0);
            set_attribute(e, "y", 0);
            set_attribute(e, "width", img->image->width.get());
            set_attribute(e, "height", img->image->height.get());
            transform_to_attr(e, img->transform.get());
            set_attribute(e, "xlink:href", img->image->to_url().toString());
        }
    }

    void write_stroke(model::Stroke* stroke, QDomElement& parent)
    {
        Style::Map style;
        style["fill"] = "none";
        if ( !animated )
        {
            style["stroke"] = styler_to_css(stroke);
            style["stroke-opacity"] = QString::number(stroke->opacity.get());
            style["stroke-width"] = QString::number(stroke->width.get());
        }
        switch ( stroke->cap.get() )
        {
            case model::Stroke::Cap::ButtCap:
                style["stroke-linecap"] = "butt";
                break;
            case model::Stroke::Cap::RoundCap:
                style["stroke-linecap"] = "round";
                break;
            case model::Stroke::Cap::SquareCap:
                style["stroke-linecap"] = "square";
                break;

        }
        switch ( stroke->join.get() )
        {
            case model::Stroke::Join::BevelJoin:
                style["stroke-linejoin"] = "bevel";
                break;
            case model::Stroke::Join::RoundJoin:
                style["stroke-linejoin"] = "round";
                break;
            case model::Stroke::Join::MiterJoin:
                style["stroke-linejoin"] = "miter";
                style["stroke-miterlimit"] = QString::number(stroke->miter_limit.get());
                break;
        }
        style["stroke-dasharray"] = "none";
        QDomElement g = write_styler_shapes(parent, stroke, style);
        if ( animated )
        {
            write_styler_attrs(g, stroke, "stroke");
            write_property(g, &stroke->width, "stroke-width");
        }
    }

    void write_fill(model::Fill* fill, QDomElement& parent)
    {
        Style::Map style;
        if ( !animated )
        {
            style["fill"] = styler_to_css(fill);
            style["fill-opacity"] = QString::number(fill->opacity.get());
        }
        style["stroke"] = "none";
        QDomElement g = write_styler_shapes(parent, fill, style);
        if ( animated )
            write_styler_attrs(g, fill, "fill");
    }

    void write_precomp_layer(model::PreCompLayer* layer, QDomElement& parent)
    {
        if ( layer->composition.get() )
        {
            timing.push_back(layer->timing.get());
            auto clip = element(defs, "clipPath");
            set_attribute(clip, "id", "clip_" + id(layer));
            set_attribute(clip, "clipPathUnits", "userSpaceOnUse");
            auto clip_rect = element(clip, "rect");
            set_attribute(clip_rect, "x", "0");
            set_attribute(clip_rect, "y", "0");
            set_attribute(clip_rect, "width", layer->size.get().width());
            set_attribute(clip_rect, "height", layer->size.get().height());

            auto e = start_layer(parent, layer);
            transform_to_attr(e, layer->transform.get());
            write_property(e, &layer->opacity, "opacity");
            write_visibility_attributes(parent, layer);
            time_stretch = layer->timing->stretch.get();
            time_start = layer->timing->start_time.get();
            write_composition(e, layer->composition.get());
            time_stretch = 1;
            time_start = 0;
            timing.pop_back();
        }
    }

    void write_repeater_vis(QDomElement& element, model::Repeater* repeater, int index, int n_copies)
    {
        element.setAttribute("display", index < repeater->copies.get() ? "block" : "none");

        float alpha_lerp = float(index) / (n_copies == 1 ? 1 : n_copies - 1);
        model::JoinAnimatables opacity({&repeater->start_opacity, &repeater->end_opacity}, model::JoinAnimatables::NoValues);
        auto opacity_func = [&alpha_lerp](float a, float b){
            return math::lerp(a, b, alpha_lerp);
        };
        set_attribute(element, "opacity", opacity.combine_current_value<float, float>(opacity_func));

        if ( animated )
        {
            int kf_count = repeater->copies.keyframe_count();
            if ( kf_count >= 2 )
            {
                AnimationData anim_display(this, {"display"}, kf_count, time_stretch, time_start);

                for ( int i = 0; i < kf_count; i++ )
                {
                    auto kf = repeater->copies.keyframe(i);
                    anim_display.add_keyframe(time_to_global(kf->time()), {index < kf->get() ? "block" : "none"}, kf->transition());
                }

                anim_display.add_dom(element);
            }

            if ( opacity.animated() )
            {
                AnimationData anim_opacity(this, {"opacity"}, opacity.keyframes().size(), time_stretch, time_start);
                for ( const auto& keyframe : opacity.keyframes() )
                {
                    anim_opacity.add_keyframe(
                        time_to_global(keyframe.time),
                        {QString::number(opacity.combine_value_at<float, float>(keyframe.time, opacity_func))},
                        keyframe.transition()
                    );
                }
            }
        }
    }

    void write_repeater(model::Repeater* repeater, QDomElement& parent, bool force_draw)
    {
        int n_copies = repeater->max_copies();
        if ( n_copies < 1 )
            return;

        QDomElement container = start_group(parent, repeater);
        QString base_id = id(repeater);
        QString prev_clone_id = base_id + "_0";
        QDomElement og = element(container, "g");
        og.setAttribute("id", prev_clone_id);
        for ( const auto& sib : repeater->affected() )
            write_shape(og, sib, force_draw);

        write_repeater_vis(og, repeater, 0, n_copies);

        for ( int i = 1; i < n_copies; i++ )
        {
            QString clone_id = base_id + "_" + QString::number(i);;
            QDomElement use = element(container, "use");
            use.setAttribute("xlink:href", "#" + prev_clone_id);
            use.setAttribute("id", clone_id);
            write_repeater_vis(use, repeater, i, n_copies);
            transform_to_attr(use, repeater->transform.get());
            prev_clone_id = clone_id;
        }
    }

    void write_shape(QDomElement& parent, model::ShapeElement* shape, bool force_draw)
    {
        if ( auto grp = qobject_cast<model::Group*>(shape) )
        {
            write_group_shape(parent, grp);
        }
        else if ( auto stroke = qobject_cast<model::Stroke*>(shape) )
        {
            if ( stroke->visible.get() )
                write_stroke(stroke, parent);
        }
        else if ( auto fill = qobject_cast<model::Fill*>(shape) )
        {
            if ( fill->visible.get() )
                write_fill(fill, parent);
        }
        else if ( auto img = qobject_cast<model::Image*>(shape) )
        {
            write_image(img, parent);
        }
        else if ( auto layer = qobject_cast<model::PreCompLayer*>(shape) )
        {
            write_precomp_layer(layer, parent);
        }
        else if ( auto repeater = qobject_cast<model::Repeater*>(shape) )
        {
            write_repeater(repeater, parent, force_draw);
        }
        else if ( force_draw )
        {
            write_shape_shape(parent, shape, {});
            write_visibility_attributes(parent, shape);
            set_attribute(parent, "id", id(shape));
        }
    }

    QDomElement write_bezier(QDomElement& parent, model::ShapeElement* shape, const Style::Map& style)
    {
        QDomElement path = element(parent, "path");
        write_style(path, style);
        QString d;
        QString nodetypes;
        std::tie(d, nodetypes) = path_data(shape->shapes(shape->time()));
        set_attribute(path, "d", d);
        set_attribute(path, "sodipodi:nodetypes", nodetypes);

        if ( animated )
        {
            std::vector<const model::AnimatableBase*> props;
            for ( auto prop : shape->properties() )
            {
                if ( prop->traits().flags & model::PropertyTraits::Animated )
                    props.push_back(static_cast<model::AnimatableBase*>(prop));
            }

            model::JoinAnimatables j(std::move(props), model::JoinAnimatables::NoValues);

            if ( j.animated() )
            {
                AnimationData data(this, {"d"}, j.keyframes().size(), time_stretch, time_start);

                for ( const auto& kf : j )
                    data.add_keyframe(time_to_global(kf.time), {path_data(shape->shapes(kf.time)).first}, kf.transition());

                data.add_dom(path);
            }
        }
        return path;
    }

    /**
     * \brief Creates a <g> element for recurse_parents
     * \param parent        DOM element to add the <g> into
     * \param ancestor      Ancestor layer (to create the <g> for)
     * \param descendant    Descendant layer
     */
    QDomElement start_layer_recurse_parents(const QDomElement& parent, model::Layer* ancestor, model::Layer* descendant)
    {
        QDomElement g = element(parent, "g");
        g.setAttribute("id", id(descendant) + "_" + id(ancestor));
        g.setAttribute("inkscape:label", QObject::tr("%1 (%2)").arg(descendant->object_name()).arg(ancestor->object_name()));
        g.setAttribute("inkscape:groupmode", "layer");
        transform_to_attr(g, ancestor->transform.get());
        return g;
    }

    /**
     * \brief Creates nested <g> elements for each layer parent (using the parent property)
     * \param parent        DOM element to add the elements into
     * \param ancestor      Ancestor layer (searched recursively for parents)
     * \param descendant    Descendant layer
     */
    QDomElement recurse_parents(const QDomElement& parent, model::Layer* ancestor, model::Layer* descendant)
    {
        if ( !ancestor->parent.get() )
            return start_layer_recurse_parents(parent, ancestor, descendant);
        return start_layer_recurse_parents(recurse_parents(parent, ancestor->parent.get(), descendant), ancestor, descendant);
    }

    void write_group_shape(QDomElement& parent, model::Group* group)
    {
        QDomElement g;
        bool has_mask = false;
        if ( auto layer = group->cast<model::Layer>() )
        {
            if ( !layer->render.get() )
                return;

            if ( layer->parent.get() )
            {
                QDomElement parent_g = recurse_parents(parent, layer->parent.get(), layer);
                g = start_layer(parent_g, group);
            }
            else
            {
                g = start_layer(parent, group);
            }

            if ( layer->mask->has_mask() )
            {
                has_mask = true;

                QDomElement clip = element(defs, "mask");
                QString mask_id = "clip_" + id(layer);
                clip.setAttribute("id", mask_id);
                clip.setAttribute("mask-type", "alpha");
                if ( layer->shapes.size() > 1 )
                    write_shape(clip, layer->shapes[0], false);

                g.setAttribute("mask", "url(#" + mask_id + ")");
            }

            if ( animated && layer->visible.get() )
            {
                auto* lay_range = layer->animation.get();
                auto* doc_range = layer->owner_composition()->animation.get();
                bool has_start = lay_range->first_frame.get() > doc_range->first_frame.get();
                bool has_end = lay_range->last_frame.get() < doc_range->last_frame.get();

                if ( has_start || has_end )
                {
                    QDomElement animation = element(g, "animate");
                    animation.setAttribute("begin", clock(ip));
                    animation.setAttribute("dur", clock(op-ip));
                    animation.setAttribute("calcMode", "discrete");
                    animation.setAttribute("attributeName", "display");
                    animation.setAttribute("repeatCount", "indefinite");
                    QString times;
                    QString vals;

                    times += "0;";

                    if ( has_start )
                    {
                        vals += "none;inline;";
                        times += unlerp_time(lay_range->first_frame.get()) + ";";
                    }
                    else
                    {
                        vals += "inline;";
                    }

                    if ( has_end )
                    {
                        vals += "none;";
                        times += unlerp_time(lay_range->last_frame.get()) + ";";
                    }

                    animation.setAttribute("values", vals);
                    animation.setAttribute("keyTimes", times);
                }
            }
        }
        else
        {
            g = start_group(parent, group);
        }

        transform_to_attr(g, group->transform.get(), group->auto_orient.get());
        write_property(g, &group->opacity, "opacity");
        write_visibility_attributes(g, group);
        write_shapes(g, group->shapes, has_mask);
    }

    template<class PropT, class Callback>
    QDomElement transform_property(
        QDomElement& e, const char* name, PropT* prop, const Callback& callback,
        const QString& path = {}, bool auto_orient = false
    )
    {
        model::JoinAnimatables j({prop}, model::JoinAnimatables::NoValues);

        auto parent = e.parentNode();
        QDomElement g = dom.createElement("g");
        parent.insertBefore(g, e);
        parent.removeChild(e);
        g.appendChild(e);

        if ( j.animated() )
        {
            AnimationData data(this, {"transform"}, j.keyframes().size(), time_stretch, time_start);

            if ( !path.isEmpty() )
            {
                for ( const auto& kf : j )
                    data.add_keyframe(time_to_global(kf.time), {""}, kf.transition());
                data.add_dom(g, "animateMotion", "", path, auto_orient);
            }
            else
            {

                for ( const auto& kf : j )
                    data.add_keyframe(time_to_global(kf.time), {callback(prop->get_at(kf.time))}, kf.transition());
                data.add_dom(g, "animateTransform", name);
            }
        }

        g.setAttribute("transform", QString("%1(%2)").arg(name).arg(callback(prop->get())));
        return g;
    }

    void transform_to_attr(QDomElement& parent, model::Transform* transf, bool auto_orient = false)
    {
        if ( animated && (transf->position.animated() || transf->scale.animated() || transf->rotation.animated() || transf->anchor_point.animated()) )
        {
            QDomElement subject = parent;
            subject = transform_property(subject, "translate", &transf->anchor_point, [](const QPointF& val){
                return QString("%1 %2").arg(-val.x()).arg(-val.y());
            });
            subject = transform_property(subject, "scale", &transf->scale, [](const QVector2D& val){
                return QString("%1 %2").arg(val.x()).arg(val.y());
            });
            subject = transform_property(subject, "rotate", &transf->rotation, [](qreal val){
                return QString::number(val);
            });
            math::bezier::MultiBezier mb;
            mb.beziers().push_back(transf->position.bezier());
            subject = transform_property(subject, "translate", &transf->position, [](const QPointF& val){
                return QString("%1 %2").arg(val.x()).arg(val.y());
            }, path_data(mb).first, auto_orient);
        }
        else
        {
            auto matr = transf->transform_matrix(transf->time());
            parent.setAttribute("transform", QString("matrix(%1, %2, %3, %4, %5, %6)")
                .arg(matr.m11())
                .arg(matr.m12())
                .arg(matr.m21())
                .arg(matr.m22())
                .arg(matr.m31())
                .arg(matr.m32())
            );
        }
    }

    void write_style(QDomElement& element, const Style::Map& s)
    {
        QString st;
        for ( auto it : s )
        {
            st.append(it.first);
            st.append(':');
            st.append(it.second);
            st.append(';');
        }
        element.setAttribute("style", st);
    }

    QDomElement start_group(QDomElement& parent, model::DocumentNode* node)
    {
        QDomElement g = element(parent, "g");
        g.setAttribute("id", id(node));
        g.setAttribute("inkscape:label", node->object_name());
        return g;
    }

    QDomElement start_layer(QDomElement& parent, model::DocumentNode* node)
    {
        auto g = start_group(parent, node);
        g.setAttribute("inkscape:groupmode", "layer");
        return g;
    }

    QString id(model::DocumentNode* node)
    {
        return node->type_name() + "_" + node->uuid.get().toString(QUuid::Id128);
    }

    /// Avoid locale nonsense by defining these functions (on ASCII chars) manually
    static constexpr bool valid_id_start(char c) noexcept
    {
        return  ( c >= 'a' && c <= 'z') ||
                ( c >= 'A' && c <= 'Z') ||
                c == '_';
    }

    static constexpr bool valid_id(char c) noexcept
    {
        return  valid_id_start(c) ||
                ( c >= '0' && c <= '9') ||
                c == '-';
    }

    void write_named_color(QDomElement& parent, model::NamedColor* color)
    {
        auto gradient = element(parent, "linearGradient");
        gradient.setAttribute("osb:paint", "solid");
        QString id = pretty_id(color->name.get(), color);
        non_uuid_ids_map[color] = id;
        gradient.setAttribute("id", id);

        auto stop = element(gradient, "stop");
        stop.setAttribute("offset", "0");
        write_property(stop, &color->color, "stop-color");
    }

    QString pretty_id(const QString& s, model::DocumentNode* node)
    {
        if ( s.isEmpty() )
            return id(node);

        QByteArray str = s.toLatin1();
        QString id_attempt;
        if ( !valid_id_start(str[0]) )
            id_attempt.push_back('_');

        for ( char c : str )
        {
            if ( c == ' ' )
                id_attempt.push_back('_');
            else if ( valid_id(c) )
                id_attempt.push_back(c);
        }

        if ( id_attempt.isEmpty() )
            return id(node);

        QString id_final = id_attempt;
        int i = 1;
        while ( non_uuid_ids.count(id_final) )
            id_final = id_attempt + QString::number(i++);

        return id_final;
    }

    template<class T>
    std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>>
    set_attribute(QDomElement& e, const QString& name, T val)
    {
        // not using e.setAttribute overloads to bypass locale settings
        e.setAttribute(name, QString::number(val));
    }

    void set_attribute(QDomElement& e, const QString& name, bool val)
    {
        e.setAttribute(name, val ? "true" : "false");
    }

    void set_attribute(QDomElement& e, const QString& name, const char* val)
    {
        e.setAttribute(name, val);
    }

    void set_attribute(QDomElement& e, const QString& name, const QString& val)
    {
        e.setAttribute(name, val);
    }


    void write_gradient_colors(QDomElement& parent, model::GradientColors* gradient)
    {
        auto e = element(parent, "linearGradient");
        QString id = pretty_id(gradient->name.get(), gradient);
        non_uuid_ids_map[gradient] = id;
        e.setAttribute("id", id);

        if ( animated && gradient->colors.keyframe_count() > 1 )
        {
            int n_stops = std::numeric_limits<int>::max();
            for ( const auto& kf : gradient->colors )
                if ( kf.get().size() < n_stops )
                    n_stops = kf.get().size();

            auto stops = gradient->colors.get();
            for ( int i = 0; i < n_stops; i++ )
            {
                AnimationData data(this, {"offset", "stop-color"}, gradient->colors.keyframe_count(), time_stretch, time_start);
                for ( const auto& kf : gradient->colors )
                {
                    auto stop = kf.get()[i];
                    data.add_keyframe(
                        time_to_global(kf.time()),
                        {QString::number(stop.first), stop.second.name()},
                        kf.transition()
                    );
                }

                auto s = element(e, "stop");
                s.setAttribute("stop-opacity", "1");
                set_attribute(s, "offset", stops[i].first);
                s.setAttribute("stop-color", stops[i].second.name());
                data.add_dom(s);
            }
        }
        else
        {
            for ( const auto& stop : gradient->colors.get() )
            {
                auto s = element(e, "stop");
                s.setAttribute("stop-opacity", "1");
                set_attribute(s, "offset", stop.first);
                s.setAttribute("stop-color", stop.second.name());
            }
        }
    }

    void write_gradient(QDomElement& parent, model::Gradient* gradient)
    {
        QDomElement e;
        if ( gradient->type.get() == model::Gradient::Radial || gradient->type.get() == model::Gradient::Conical )
        {
            e = element(parent, "radialGradient");
            write_properties(e, {&gradient->start_point}, {"cx", "cy"}, &Private::callback_point);
            write_properties(e, {&gradient->highlight}, {"fx", "fy"}, &Private::callback_point);

            write_properties(e, {&gradient->start_point, &gradient->end_point}, {"r"},
                [](const std::vector<QVariant>& values) -> std::vector<QString> {
                    return { QString::number(
                        math::length(values[1].toPointF() - values[0].toPointF())
                    )};
            });
        }
        else
        {
            e = element(parent, "linearGradient");
            write_properties(e, {&gradient->start_point}, {"x1", "y1"}, &Private::callback_point);
            write_properties(e, {&gradient->end_point}, {"x2", "y2"}, &Private::callback_point);
        }

        QString id = pretty_id(gradient->name.get(), gradient);
        non_uuid_ids_map[gradient] = id;
        e.setAttribute("id", id);
        e.setAttribute("gradientUnits", "userSpaceOnUse");

        auto it = non_uuid_ids_map.find(gradient->colors.get());
        if ( it != non_uuid_ids_map.end() )
            e.setAttribute("xlink:href", "#" + it->second);
    }

    QString clock(model::FrameTime time)
    {
        return QString::number(time / fps, 'f');
    }

    std::vector<model::StretchableTime*> timing;
    QDomDocument dom;
    qreal fps = 60;
    qreal ip = 0;
    qreal op = 60;
    bool at_start = true;
    std::set<QString> non_uuid_ids;
    std::map<model::DocumentNode*, QString> non_uuid_ids_map;
    AnimationType animated;
    QDomElement svg;
    QDomElement defs;
    CssFontType font_type;
    qreal time_stretch = 1;
    model::FrameTime time_start = 0;
};


io::svg::SvgRenderer::SvgRenderer(AnimationType animated, CssFontType font_type)
    : d(std::make_unique<Private>())
{
    d->animated = animated;
    d->font_type = font_type;
    d->svg = d->dom.createElement("svg");
    d->dom.appendChild(d->svg);
    d->svg.setAttribute("xmlns", detail::xmlns.at("svg"));
    for ( const auto& p : detail::xmlns )
    {
        if ( !p.second.contains("android") )
            d->svg.setAttribute("xmlns:" + p.first, p.second);
    }

    d->write_style(d->svg, {
        {"fill", "none"},
        {"stroke", "none"}
    });
    d->svg.setAttribute("inkscape:export-xdpi", "96");
    d->svg.setAttribute("inkscape:export-ydpi", "96");
    d->svg.setAttribute("version", "1.1");
}

io::svg::SvgRenderer::~SvgRenderer()
{
}

void io::svg::SvgRenderer::write_composition(model::Composition* comp)
{
    d->collect_defs(comp);
    auto g = d->start_layer(d->svg, comp);
    d->write_composition(g, comp);
}


void io::svg::SvgRenderer::write_main(model::Composition* comp)
{
    if ( d->at_start )
    {
        QString w  = QString::number(comp->width.get());
        QString h = QString::number(comp->height.get());
        d->svg.setAttribute("width", w);
        d->svg.setAttribute("height", h);
        d->svg.setAttribute("viewBox", QString("0 0 %1 %2").arg(w).arg(h));
        d->svg.appendChild(d->dom.createElement("title")).appendChild(d->dom.createTextNode(comp->name.get()));
        write_composition(comp);
    }
    else
    {
        write_composition(comp);
    }
}

void io::svg::SvgRenderer::write_shape(model::ShapeElement* shape)
{
    d->collect_defs(shape->owner_composition());
    d->write_shape(d->svg, shape, true);
}

void io::svg::SvgRenderer::write_node(model::DocumentNode* node)
{
    if ( auto co = qobject_cast<model::Composition*>(node) )
        write_main(co);
    else if ( auto sh = qobject_cast<model::ShapeElement*>(node) )
        write_shape(sh);
}

QDomDocument io::svg::SvgRenderer::dom() const
{
    return d->dom;
}

void io::svg::SvgRenderer::write(QIODevice* device, bool indent)
{
    device->write(d->dom.toByteArray(indent ? 4 : -1));
}

glaxnimate::io::svg::CssFontType glaxnimate::io::svg::SvgRenderer::suggested_type(model::EmbeddedFont* font)
{
    if ( !font->css_url.get().isEmpty() )
        return CssFontType::Link;
    if ( !font->source_url.get().isEmpty() )
        return CssFontType::FontFace;
    if ( !font->data.get().isEmpty() )
        return CssFontType::Embedded;
    return CssFontType::None;
}

static char bezier_node_type(const math::bezier::Point& p)
{
    switch ( p.type )
    {
        case math::bezier::PointType::Smooth:
            return 's';
        case math::bezier::PointType::Symmetrical:
            return 'z';
        case math::bezier::PointType::Corner:
        default:
            return 'c';
    }
}

std::pair<QString, QString> glaxnimate::io::svg::path_data(const math::bezier::MultiBezier& shape)
{
    QString d;
    QString nodetypes;
    for ( const math::bezier::Bezier& b : shape.beziers() )
    {
        if ( b.empty() )
            continue;

        d += QString("M %1,%2 C").arg(b[0].pos.x()).arg(b[0].pos.y());
        nodetypes += bezier_node_type(b[0]);

        for ( int i = 1; i < b.size(); i++ )
        {
            d += QString(" %1,%2 %3,%4 %5,%6")
                .arg(b[i-1].tan_out.x()).arg(b[i-1].tan_out.y())
                .arg(b[i].tan_in.x()).arg(b[i].tan_in.y())
                .arg(b[i].pos.x()).arg(b[i].pos.y())
            ;
            nodetypes += bezier_node_type(b[i]);
        }

        if ( b.closed() )
        {
            d += QString(" %1,%2 %3,%4 %5,%6")
                .arg(b.back().tan_out.x()).arg(b.back().tan_out.y())
                .arg(b[0].tan_in.x()).arg(b[0].tan_in.y())
                .arg(b[0].pos.x()).arg(b[0].pos.y())
            ;
            d += " Z";
        }
    }
    return {d, nodetypes};
}
