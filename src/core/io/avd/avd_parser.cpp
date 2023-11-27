/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "avd_parser.hpp"

#include <QPalette>

#include "io/svg/svg_parser_private.hpp"
#include "model/shapes/trim.hpp"

using namespace glaxnimate::io::svg;
using namespace glaxnimate::io::svg::detail;

class glaxnimate::io::avd::AvdParser::Private : public svg::detail::SvgParserPrivate
{
public:
    Private(
        const QDir& resource_path,
        model::Document* document,
        const std::function<void(const QString&)>& on_warning,
        ImportExport* io,
        QSize forced_size,
        model::FrameTime default_time
    ) : SvgParserPrivate(document, on_warning, io, forced_size, default_time),
        resource_path(resource_path)
    {}

protected:
    void on_parse_prepare(const QDomElement&) override
    {
        for ( const auto& p : shape_parsers )
            to_process += dom.elementsByTagName(p.first).count();


        for ( const auto& target : ElementRange(dom.elementsByTagName("target"_qs)) )
        {
            QString name = target.attribute("name"_qs);
            if ( name.isEmpty() )
                continue;

            for ( const auto& attr : ElementRange(target) )
            {
                if ( attr.tagName() != "attr"_qs || !attr.attribute("name"_qs).endsWith("animation"_qs) )
                    continue;

                auto iter = animations.find(name);
                if ( iter == animations.end() )
                    iter = animations.insert({name, {}}).first;

                auto& props = iter->second;

                for ( const auto& anim : ElementRange(attr.elementsByTagName("objectAnimator"_qs)) )
                {
                    parse_animator(props, anim);
                }
            }
        }
    }

    QSizeF get_size(const QDomElement& svg) override
    {
        return {
            len_attr(svg, "width"_qs, size.width()),
            len_attr(svg, "height"_qs, size.height())
        };
    }

    void on_parse(const QDomElement& root) override
    {
        static const Style default_style(Style::Map{
            {"fillColor"_qs, "black"_qs},
        });

        if ( root.tagName() == "vector"_qs )
        {
            parse_vector({root, &main->shapes, default_style, false});
        }
        else
        {
            if ( root.hasAttribute("drawable"_qs) )
            {
                if ( auto res = get_resource(root.attribute("drawable"_qs)) )
                {
                    if ( res->element.tagName() == "vector"_qs )
                        parse_vector({res->element, &main->shapes, default_style, false});
                }
            }

            for ( const auto& ch : ElementRange(root) )
            {
                if ( ch.tagName() == "attr"_qs && ch.attribute("name"_qs).endsWith("drawable"_qs) )
                {
                    for ( const auto& e : ElementRange(ch) )
                        if ( e.tagName() == "vector"_qs )
                            parse_vector({e, &main->shapes, default_style, false});
                }
            }
        }

        main->name.set(
            attr(root, "android"_qs, "name"_qs, ""_qs)
        );
    }

    void parse_shape(const ParseFuncArgs& args) override
    {
        auto it = shape_parsers.find(args.element.tagName());
        if ( it != shape_parsers.end() )
        {
            mark_progress();
            (this->*it->second)(args);
        }
    }

private:
    struct Resource
    {
        QString name;
        QDomElement element;
        model::Asset* asset = nullptr;
    };

    void parse_vector(const ParseFuncArgs& args)
    {
        QPointF pos;
        QVector2D scale{1, 1};

        model::Layer* layer = add_layer(args.shape_parent);
        set_name(layer, args.element);

        if ( args.element.hasAttribute("viewportWidth"_qs) && args.element.hasAttribute("viewportHeight"_qs) )
        {
            qreal vbw = len_attr(args.element, "viewportWidth"_qs);
            qreal vbh = len_attr(args.element, "viewportHeight"_qs);

            if ( !forced_size.isValid() )
            {
                if ( !args.element.hasAttribute("width"_qs) )
                    size.setWidth(vbw);
                if ( !args.element.hasAttribute("height"_qs) )
                    size.setHeight(vbh);
            }

            if ( vbw != 0 && vbh != 0 )
            {
                scale = QVector2D(size.width() / vbw, size.height() / vbh);

                if ( forced_size.isValid() )
                {
                    auto single = qMin(scale.x(), scale.y());
                    scale = QVector2D(single, single);
                }
            }
        }

        layer->transform.get()->position.set(-pos);
        layer->transform.get()->scale.set(scale);

        parse_children({args.element, &layer->shapes, args.parent_style, false});
    }

    void parse_animator(AnimateParser::AnimatedProperties& props, const QDomElement& anim)
    {
        model::FrameTime start_time = qRound(anim.attribute("startOffset"_qs, "0"_qs).toDouble() / 1000 * animate_parser.fps);
        model::FrameTime end_time = qRound(start_time + anim.attribute("duration"_qs, "0"_qs).toDouble() / 1000 * animate_parser.fps);
        animate_parser.register_time_range(start_time, end_time);
        std::vector<AnimatedProperty*> updated_props;

        QString name = anim.attribute("propertyName"_qs);
        if ( !name.isEmpty() )
        {
            auto& prop = props.properties[name];
            updated_props.push_back(&prop);
            parse_animated_prop(prop, name, anim, start_time, end_time);
        }

        for ( const auto& value_holder : ElementRange(anim) )
        {
            if ( value_holder.tagName() != "propertyValuesHolder"_qs )
                continue;

            name = value_holder.attribute("propertyName"_qs);
            if ( !name.isEmpty() )
            {
                auto& prop = props.properties[name];
                updated_props.push_back(&prop);
                parse_animated_prop(prop, name, value_holder, start_time, end_time);
            }
        }

        for ( auto prop : updated_props )
            prop->sort();
    }

    model::KeyframeTransition interpolator(const QString& interpolator)
    {
        using Type = model::KeyframeTransition::Descriptive;

        if ( interpolator == "@android:interpolator/fast_out_slow_in"_qs )
            return model::KeyframeTransition(Type::Fast, Type::Ease);
        if ( interpolator == "@android:interpolator/fast_out_linear_in"_qs )
            return model::KeyframeTransition(Type::Fast, Type::Linear);
        if ( interpolator == "@android:interpolator/linear_out_slow_in"_qs )
            return model::KeyframeTransition(Type::Linear, Type::Ease);
        if ( interpolator == "@android:anim/accelerate_decelerate_interpolator"_qs )
            return model::KeyframeTransition(Type::Ease, Type::Ease);
        if ( interpolator == "@android:anim/accelerate_interpolator"_qs )
            return model::KeyframeTransition(Type::Ease, Type::Fast);
        if ( interpolator == "@android:anim/decelerate_interpolator"_qs )
            return model::KeyframeTransition(Type::Fast, Type::Ease);
        if ( interpolator == "@android:anim/linear_interpolator"_qs )
            return model::KeyframeTransition(Type::Linear, Type::Linear);

        // TODO?
        // @android:anim/anticipate_interpolator
        // @android:anim/overshoot_interpolator
        // @android:anim/bounce_interpolator
        // @android:anim/anticipate_overshoot_interpolator
        if ( interpolator != ""_qs )
            warning(QObject::tr("Unknown interpolator %s").arg(interpolator));

        return model::KeyframeTransition(Type::Ease, Type::Ease);
    }

    ValueVariant parse_animated_value(const QString& value, ValueVariant::Type type)
    {
        switch ( type )
        {
            case ValueVariant::Vector:
                return value.toDouble();
            case ValueVariant::Bezier:
                return PathDParser(value).parse();
            case ValueVariant::String:
                return value;
            case ValueVariant::Color:
                return parse_color(value);
        }

        return {};
    }

    void parse_animated_prop(
        AnimatedProperty& prop,
        const QString& name,
        const QDomElement& value_holder,
        model::FrameTime start_time,
        model::FrameTime end_time
    )
    {
        static model::KeyframeTransition transition;

        ValueVariant::Type type = ValueVariant::Vector;
        if ( name == "pathData"_qs )
            type = ValueVariant::Bezier;
        else if ( name.endsWith("Color"_qs) )
            type = ValueVariant::Color;

        if ( value_holder.hasAttribute("valueFrom"_qs) )
        {
            prop.keyframes.push_back({
                start_time,
                parse_animated_value(value_holder.attribute("valueFrom"_qs), type),
                interpolator(value_holder.attribute("interpolator"_qs))
            });
        }

        if ( value_holder.hasAttribute("valueTo"_qs) )
        {
            prop.keyframes.push_back({
                end_time,
                parse_animated_value(value_holder.attribute("valueTo"_qs), type),
                model::KeyframeTransition(model::KeyframeTransition::Ease)
            });
        }

        for ( const auto& kf : ElementRange(value_holder) )
        {
            if ( kf.tagName() != "keyframe"_qs )
                continue;

            auto fraction = kf.attribute("fraction"_qs).toDouble();

            prop.keyframes.push_back({
                math::lerp(start_time, end_time, fraction),
                parse_animated_value(kf.attribute("value"_qs), type),
                interpolator(kf.attribute("interpolator"_qs))
            });
        }
    }

    void add_shapes(const ParseFuncArgs& args, ShapeCollection&& shapes)
    {
        Style style = parse_style(args.element, args.parent_style);
        auto group = std::make_unique<model::Group>(document);

//         apply_common_style(group.get(), args.element, style);

        set_name(group.get(), args.element);

        add_style_shapes(args, &group->shapes, style);

        for ( auto& shape : shapes )
            group->shapes.insert(std::move(shape));

        args.shape_parent->insert(std::move(group));
    }

    Style parse_style(const QDomElement& element, const Style& parent_style)
    {
        Style style = parent_style;

        for ( const auto& domnode : ItemCountRange(element.attributes()) )
        {
            auto attr = domnode.toAttr();
            if ( style_atrrs.count(attr.name()) )
                style[attr.name()] = attr.value();
        }

        for ( const auto& child : ItemCountRange(element.childNodes()) )
        {
            if ( child.isElement() )
            {
                auto attr = child.toElement();
                if ( attr.tagName() == "attr"_qs )
                {
                    auto attr_name = attr.attribute("name"_qs).split(":"_qs).back();
                    for ( const auto& grandchild : ItemCountRange(child.childNodes()) )
                    {
                        if ( grandchild.isElement() )
                        {
                            style[attr_name] = add_as_resource(grandchild.toElement());
                            break;
                        }
                    }
                }
            }
        }

        return style;
    }

    void set_name(model::DocumentNode* node, const QDomElement& element)
    {
        QString name = attr(element, ""_qs, "name"_qs, node->type_name_human());
        node->name.set(name);
    }

    void add_style_shapes(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        add_fill(args, shapes, style);
        add_stroke(args, shapes, style);
        if ( style.contains("trimPathEnd"_qs) || style.contains("trimPathStart"_qs) )
            add_trim(args, shapes, style);
    }

    QColor parse_color(const QString& color)
    {
        if ( !color.isEmpty() && color[0] == '#'_qc )
        {
            if ( color.size() == 5 )
                return svg::parse_color("#"_qs + color.mid(2) + color[1]);
            if ( color.size() == 9 )
                return svg::parse_color("#"_qs + color.mid(3) + color.mid(1, 2));
        }

        return svg::parse_color(color);
    }

    void set_styler_style(model::Styler* styler, const QString& color)
    {
        if ( color.isEmpty() )
        {
            styler->visible.set(false);
        }
        else if ( color[0] == '@'_qc )
        {
            auto res = get_resource(color);
            if ( res && res->element.tagName() == "gradient"_qs )
                styler->use.set(parse_gradient(res));
        }
        else if ( color[0] == '?'_qc )
        {
            styler->use.set(color_from_theme(color));
        }
        else
        {
            styler->color.set(parse_color(color));
        }

    }

    void add_stroke(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        auto stroke = std::make_unique<model::Stroke>(document);
        set_styler_style(stroke.get(), style.get("strokeColor"_qs, ""_qs));
        stroke->opacity.set(percent_1(style.get("strokeAlpha"_qs, "1"_qs)));

        stroke->width.set(parse_unit(style.get("strokeWidth"_qs, "1"_qs)));

        stroke->cap.set(line_cap(style.get("strokeLineCap"_qs, "butt"_qs)));
        stroke->join.set(line_join(style.get("strokeLineJoin"_qs, "butt"_qs)));
        stroke->miter_limit.set(parse_unit(style.get("strokeMiterLimit"_qs, "4"_qs)));

        auto anim = get_animations(args.element);
        for ( const auto& kf : anim.single("strokeColor"_qs) )
            stroke->color.set_keyframe(kf.time, kf.values.color())->set_transition(kf.transition);

        for ( const auto& kf : anim.single("strokeAlpha"_qs) )
            stroke->opacity.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        for ( const auto& kf : anim.single("strokeWidth"_qs) )
            stroke->width.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        shapes->insert(std::move(stroke));
    }

    const AnimateParser::AnimatedProperties& get_animations(const QDomElement& element)
    {
        auto name = element.attribute("name"_qs);
        return animations[name];
    }

    void add_fill(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        auto fill = std::make_unique<model::Fill>(document);
        set_styler_style(fill.get(), style.get("fillColor"_qs, ""_qs));
        fill->opacity.set(percent_1(style.get("fillAlpha"_qs, "1"_qs)));

        if ( style.get("fillType"_qs, ""_qs) == "evenOdd"_qs )
            fill->fill_rule.set(model::Fill::EvenOdd);

        auto anim = get_animations(args.element);
        for ( const auto& kf : anim.single("fillColor"_qs) )
            fill->color.set_keyframe(kf.time, kf.values.color())->set_transition(kf.transition);

        for ( const auto& kf : anim.single("fillAlpha"_qs) )
            fill->opacity.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        shapes->insert(std::move(fill));
    }

    void add_trim(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        auto trim = std::make_unique<model::Trim>(document);

        trim->start.set(percent_1(style.get("trimPathStart"_qs, "1"_qs)));
        trim->end.set(percent_1(style.get("trimPathEnd"_qs, "1"_qs)));
        trim->offset.set(percent_1(style.get("trimPathOffset"_qs, "1"_qs)));

        auto anim = get_animations(args.element);

        for ( const auto& kf : anim.single("trimPathStart"_qs) )
            trim->start.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        for ( const auto& kf : anim.single("trimPathEnd"_qs) )
            trim->end.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        for ( const auto& kf : anim.single("trimPathOffset"_qs) )
            trim->offset.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        shapes->insert(std::move(trim));
    }

    model::Gradient* parse_gradient(Resource* res)
    {
        if ( res->element.tagName() != "gradient"_qs )
            return nullptr;

        if ( res->asset )
            return res->asset->cast<model::Gradient>();

        // Load colors
        auto colors = document->assets()->add_gradient_colors();

        QGradientStops stops;
        if ( res->element.hasAttribute("startColor"_qs) )
        stops.push_back({0.0, parse_color(res->element.attribute("startColor"_qs))});
        if ( res->element.hasAttribute("centerColor"_qs) )
            stops.push_back({0.5, parse_color(res->element.attribute("centerColor"_qs))});
        if ( res->element.hasAttribute("endColor"_qs) )
            stops.push_back({1.0, parse_color(res->element.attribute("endColor"_qs))});

        for ( QDomElement e : ElementRange(res->element.childNodes()) )
        {
            if ( e.tagName() == "item"_qs )
                stops.push_back({
                    e.attribute("offset"_qs, "0"_qs).toDouble(),
                    parse_color(e.attribute("color"_qs))
                });
        }

        colors->colors.set(stops);

        // Load gradient
        auto gradient = document->assets()->add_gradient();
        gradient->colors.set(colors);

        QString type = res->element.attribute("type"_qs, "linear"_qs);
        if ( type == "linear"_qs )
            gradient->type.set(model::Gradient::Linear);
        else if ( type == "radial"_qs )
            gradient->type.set(model::Gradient::Radial);
        else if ( type == "sweeo"_qs )
            gradient->type.set(model::Gradient::Conical);

        gradient->start_point.set({
            len_attr(res->element, "startX"_qs),
            len_attr(res->element, "startY"_qs),
        });

        gradient->end_point.set({
            len_attr(res->element, "endX"_qs),
            len_attr(res->element, "endY"_qs),
        });

        // TODO center / radius


        res->asset = gradient;
        return gradient;
    }

    void parse_transform(model::Transform* trans, const ParseFuncArgs& args)
    {
        QPointF anchor = {
            len_attr(args.element, "pivotX"_qs),
            len_attr(args.element, "pivotY"_qs),
        };

        trans->anchor_point.set(anchor);
        trans->position.set(anchor + QPointF{
            len_attr(args.element, "translateX"_qs),
            len_attr(args.element, "translateY"_qs),
        });

        trans->scale.set(QVector2D(
            percent_1(args.element.attribute("scaleX"_qs, "1"_qs)),
            percent_1(args.element.attribute("scaleY"_qs, "1"_qs))
        ));

        trans->rotation.set(args.element.attribute("rotation"_qs, "0"_qs).toDouble());

        auto anim = get_animations(args.element);

        for ( const auto& kf : anim.joined({"pivotX"_qs, "pivotY"_qs, "translateX"_qs, "translateY"_qs}) )
        {
            anchor = QPointF(kf.values[0].scalar(), kf.values[1].scalar());
            trans->anchor_point.set_keyframe(kf.time, anchor)->set_transition(kf.transition);
            QPointF pos(kf.values[2].scalar(), kf.values[3].scalar());
            trans->position.set_keyframe(kf.time, anchor + pos)->set_transition(kf.transition);
        }

        for ( const auto& kf : anim.joined({"scaleX"_qs, "scaleY"_qs}) )
        {
            QVector2D scale(kf.values[0].scalar(), kf.values[1].scalar());
            trans->scale.set_keyframe(kf.time, scale)->set_transition(kf.transition);
        }

        for ( const auto& kf : anim.single("rotation"_qs) )
            trans->rotation.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);
    }

    std::unique_ptr<model::Group> parse_clip(const QDomElement& element)
    {
        auto clip = std::make_unique<model::Group>(document);
        set_name(clip.get(), element);

        QString d = element.attribute("pathData"_qs);
        math::bezier::MultiBezier bez = PathDParser(d).parse();

        auto fill = std::make_unique<model::Fill>(document);
        fill->color.set(QColor(255, 255, 255));
        clip->shapes.insert(std::move(fill));

        std::vector<model::Path*> shapes;
        for ( const auto& bezier : bez.beziers() )
        {
            auto shape = std::make_unique<model::Path>(document);
            shape->shape.set(bezier);
            shape->closed.set(bezier.closed());
            shapes.push_back(shape.get());
            clip->shapes.insert(std::move(shape));
        }

        path_animation(shapes, get_animations(element), "pathData"_qs);

        return clip;
    }

    void parseshape_group(const ParseFuncArgs& args)
    {
        std::unique_ptr<model::Group> clip;

        for ( auto e : ElementRange(args.element.elementsByTagName("clip-path"_qs)) )
        {
            clip = parse_clip(e);
            break;
        }

        model::Group* group = nullptr;
        if ( clip )
        {
            auto obj = std::make_unique<model::Layer>(document);
            group = obj.get();
            args.shape_parent->insert(std::move(obj));
        }
        else
        {
            auto obj = std::make_unique<model::Group>(document);
            group = obj.get();
            args.shape_parent->insert(std::move(obj));
        }

        set_name(group, args.element);

        parse_transform(group->transform.get(), args);
        parse_children({args.element, &group->shapes, args.parent_style, true});
    }

    void parseshape_path(const ParseFuncArgs& args)
    {
        QString d = args.element.attribute("pathData"_qs);
        math::bezier::MultiBezier bez = PathDParser(d).parse();

        ShapeCollection shapes;
        std::vector<model::Path*> paths;
        for ( const auto& bezier : bez.beziers() )
        {
            auto shape = push<model::Path>(shapes);
            shape->shape.set(bezier);
            shape->closed.set(bezier.closed());
            paths.push_back(shape);
        }
        add_shapes(args, std::move(shapes));

        path_animation(paths, get_animations(args.element), "pathData"_qs);
    }

    Resource* get_resource(const QString& id)
    {
        auto iter = resources.find(id);
        if ( iter != resources.end() )
            return &iter->second;

        if ( resource_path.isRoot() || id.isEmpty() || id[0] != '@'_qc || id.back() == '\0'_qc )
        {
            warning(QObject::tr("Unkown resource id %1").arg(id));
            return {};
        }

        QString path = resource_path.filePath(id.mid(1) + ".xml"_qs);
        QFile resource_file(path);
        if ( !resource_file.open(QIODevice::ReadOnly) )
        {
            warning(QObject::tr("Could not read file %1").arg(path));
            warning(QObject::tr("Could not load resource %1").arg(id));
            return {};
        }

        SvgParseError err;
        QDomDocument resource_dom;
        if ( !resource_dom.setContent(&resource_file, true, &err.message, &err.line, &err.column) )
        {
            warning(err.formatted(path));
            warning(QObject::tr("Could not load resource %1").arg(id));
            return {};
        }

        iter = resources.insert({id, {id, resource_dom.documentElement()}}).first;
        return &iter->second;
    }

    QString add_as_resource(const QDomElement& e)
    {
        internal_resource_id++;
        QString id = QStringLiteral("@(internal)%1").arg(internal_resource_id);
        id.push_back(QChar(0));
        resources[id] = {e.tagName(), e};
        return id;
    }

    model::NamedColor* color_from_theme(const QString& color)
    {
        QString norm_name;
        if ( color.contains("/"_qs) )
            norm_name = color.split("/"_qs).back();
        else
            norm_name = color.mid(1);

        auto iter = palette_colors.find(norm_name);
        if ( iter != palette_colors.end() )
            return iter->second;

        QColor col = Qt::black;
        auto it2 = theme_colors.find(norm_name);
        if ( it2 != theme_colors.end() )
            col = it2->second;

        auto asset = document->assets()->add_color(col);
        palette_colors.emplace(norm_name, asset);
        return asset;
    }

    QDir resource_path;
    std::map<QString, Resource> resources;
    int internal_resource_id = 0;
    std::map<QString, model::NamedColor*> palette_colors;
    std::map<QString, AnimateParser::AnimatedProperties> animations;

    static const std::map<QString, void (Private::*)(const ParseFuncArgs&)> shape_parsers;
    static const std::unordered_set<QString> style_atrrs;
    static const std::unordered_map<QString, QString> theme_colors;
};

const std::map<QString, void (glaxnimate::io::avd::AvdParser::Private::*)(const glaxnimate::io::avd::AvdParser::Private::ParseFuncArgs&)> glaxnimate::io::avd::AvdParser::Private::shape_parsers = {
    {"group"_qs,       &glaxnimate::io::avd::AvdParser::Private::parseshape_group},
    {"path"_qs,        &glaxnimate::io::avd::AvdParser::Private::parseshape_path},
};

const std::unordered_set<QString> glaxnimate::io::avd::AvdParser::Private::style_atrrs = {
    "fillColor"_qs, "fillAlpha"_qs, "fillType"_qs,
    "strokeColor"_qs, "strokeAlpha"_qs, "strokeWidth"_qs, "strokeLineCap"_qs, "strokeLineJoin"_qs, "strokeMiterLimit"_qs,
    "trimPathStart"_qs, "trimPathEnd"_qs, "trimPathOffset"_qs,
};

glaxnimate::io::avd::AvdParser::AvdParser(
    QIODevice* device,
    const QDir& resource_path,
    model::Document* document,
    const std::function<void(const QString&)>& on_warning,
    ImportExport* io,
    QSize forced_size,
    model::FrameTime default_time
)
    : d(std::make_unique<Private>(resource_path, document, on_warning, io, forced_size, default_time))
{
    d->load(device);
}

glaxnimate::io::avd::AvdParser::~AvdParser() = default;

void glaxnimate::io::avd::AvdParser::parse_to_document()
{
    d->parse();
}

/// Based on the material theme
/// Extracted using https://gitlab.com/-/snippets/2502132
const std::unordered_map<QString, QString> glaxnimate::io::avd::AvdParser::Private::theme_colors = {
    {"colorForeground"_qs, "#ffffffff"_qs},
    {"colorForegroundInverse"_qs, "#ff000000"_qs},
    {"colorBackground"_qs, "#ff303030"_qs},
    {"colorBackgroundFloating"_qs, "#ff424242"_qs},
    {"colorError"_qs, "#ff7043"_qs},
    {"opacityListDivider"_qs, "#1f000000"_qs},
    {"textColorPrimary"_qs, "#ff000000"_qs},
    {"textColorSecondary"_qs, "#ff000000"_qs},
    {"textColorHighlight"_qs, "#ffffffff"_qs},
    {"textColorHighlightInverse"_qs, "#ffffffff"_qs},
    {"navigationBarColor"_qs, "#ff000000"_qs},
    {"panelColorBackground"_qs, "#000"_qs},
    {"colorPrimaryDark"_qs, "#ff000000"_qs},
    {"colorPrimary"_qs, "#ff212121"_qs},
    {"colorAccent"_qs, "#ff80cbc4"_qs},
    {"tooltipForegroundColor"_qs, "#ff000000"_qs},
    {"colorPopupBackground"_qs, "#ff303030"_qs},
    {"colorListDivider"_qs, "#ffffffff"_qs},
    {"textColorLink"_qs, "#ff80cbc4"_qs},
    {"textColorLinkInverse"_qs, "#ff80cbc4"_qs},
    {"editTextColor"_qs, "#ff000000"_qs},
    {"windowBackground"_qs, "#ff303030"_qs},
    {"statusBarColor"_qs, "#ff000000"_qs},
    {"panelBackground"_qs, "#ff303030"_qs},
    {"panelColorForeground"_qs, "#ff000000"_qs},
    {"detailsElementBackground"_qs, "#ff303030"_qs},
    {"actionMenuTextColor"_qs, "#ff000000"_qs},
    {"colorEdgeEffect"_qs, "#ff212121"_qs},
    {"colorControlNormal"_qs, "#ff000000"_qs},
    {"colorControlActivated"_qs, "#ff80cbc4"_qs},
    {"colorProgressBackgroundNormal"_qs, "#ff000000"_qs},
};
