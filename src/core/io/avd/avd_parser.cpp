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


        for ( const auto& target : ElementRange(dom.elementsByTagName("target")) )
        {
            QString name = target.attribute("name");
            if ( name.isEmpty() )
                continue;

            for ( const auto& attr : ElementRange(target) )
            {
                if ( attr.tagName() != "attr" || !attr.attribute("name").endsWith("animation") )
                    continue;

                auto iter = animations.find(name);
                if ( iter == animations.end() )
                    iter = animations.insert({name, {}}).first;

                auto& props = iter->second;

                for ( const auto& anim : ElementRange(attr.elementsByTagName("objectAnimator")) )
                {
                    parse_animator(props, anim);
                }
            }
        }
    }

    QSizeF get_size(const QDomElement& svg) override
    {
        return {
            len_attr(svg, "width", size.width()),
            len_attr(svg, "height", size.height())
        };
    }

    void on_parse(const QDomElement& root) override
    {
        static const Style default_style(Style::Map{
            {"fillColor", "black"},
        });

        if ( root.tagName() == "vector" )
        {
            parse_vector({root, &main->shapes, default_style, false});
        }
        else
        {
            if ( root.hasAttribute("drawable") )
            {
                if ( auto res = get_resource(root.attribute("drawable")) )
                {
                    if ( res->element.tagName() == "vector" )
                        parse_vector({res->element, &main->shapes, default_style, false});
                }
            }

            for ( const auto& ch : ElementRange(root) )
            {
                if ( ch.tagName() == "attr" && ch.attribute("name").endsWith("drawable") )
                {
                    for ( const auto& e : ElementRange(ch) )
                        if ( e.tagName() == "vector" )
                            parse_vector({e, &main->shapes, default_style, false});
                }
            }
        }

        main->name.set(
            attr(root, "android", "name", "")
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

        if ( args.element.hasAttribute("viewportWidth") && args.element.hasAttribute("viewportHeight") )
        {
            qreal vbw = len_attr(args.element, "viewportWidth");
            qreal vbh = len_attr(args.element, "viewportHeight");

            if ( !forced_size.isValid() )
            {
                if ( !args.element.hasAttribute("width") )
                    size.setWidth(vbw);
                if ( !args.element.hasAttribute("height") )
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
        model::FrameTime start_time = qRound(anim.attribute("startOffset", "0").toDouble() / 1000 * animate_parser.fps);
        model::FrameTime end_time = qRound(start_time + anim.attribute("duration", "0").toDouble() / 1000 * animate_parser.fps);
        std::vector<AnimatedProperty*> updated_props;

        QString name = anim.attribute("propertyName");
        if ( !name.isEmpty() )
        {
            auto& prop = props.properties[name];
            updated_props.push_back(&prop);
            parse_animated_prop(prop, name, anim, start_time, end_time);
        }

        for ( const auto& value_holder : ElementRange(anim) )
        {
            if ( value_holder.tagName() != "propertyValuesHolder" )
                continue;

            name = value_holder.attribute("propertyName");
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

        if ( interpolator == "@android:interpolator/fast_out_slow_in" )
            return model::KeyframeTransition(Type::Fast, Type::Ease);
        if ( interpolator == "@android:interpolator/fast_out_linear_in" )
            return model::KeyframeTransition(Type::Fast, Type::Linear);
        if ( interpolator == "@android:interpolator/linear_out_slow_in" )
            return model::KeyframeTransition(Type::Linear, Type::Ease);
        if ( interpolator == "@android:anim/accelerate_decelerate_interpolator" )
            return model::KeyframeTransition(Type::Ease, Type::Ease);
        if ( interpolator == "@android:anim/accelerate_interpolator" )
            return model::KeyframeTransition(Type::Ease, Type::Fast);
        if ( interpolator == "@android:anim/decelerate_interpolator" )
            return model::KeyframeTransition(Type::Fast, Type::Ease);
        if ( interpolator == "@android:anim/linear_interpolator" )
            return model::KeyframeTransition(Type::Linear, Type::Linear);

        // TODO?
        // @android:anim/anticipate_interpolator
        // @android:anim/overshoot_interpolator
        // @android:anim/bounce_interpolator
        // @android:anim/anticipate_overshoot_interpolator
        if ( interpolator != "" )
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
        if ( name == "pathData" )
            type = ValueVariant::Bezier;
        else if ( name.endsWith("Color") )
            type = ValueVariant::Color;

        if ( value_holder.hasAttribute("valueFrom") )
        {
            prop.keyframes.push_back({
                start_time,
                parse_animated_value(value_holder.attribute("valueFrom"), type),
                interpolator(value_holder.attribute("interpolator"))
            });
        }

        if ( value_holder.hasAttribute("valueTo") )
        {
            prop.keyframes.push_back({
                end_time,
                parse_animated_value(value_holder.attribute("valueTo"), type),
                model::KeyframeTransition(model::KeyframeTransition::Ease)
            });
        }

        for ( const auto& kf : ElementRange(value_holder) )
        {
            if ( kf.tagName() != "keyframe" )
                continue;

            auto fraction = kf.attribute("fraction").toDouble();

            prop.keyframes.push_back({
                math::lerp(start_time, end_time, fraction),
                parse_animated_value(kf.attribute("value"), type),
                interpolator(kf.attribute("interpolator"))
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
                if ( attr.tagName() == "attr" )
                {
                    auto attr_name = attr.attribute("name").split(":").back();
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
        QString name = attr(element, "", "name", node->type_name_human());
        node->name.set(name);
    }

    void add_style_shapes(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        add_fill(args, shapes, style);
        add_stroke(args, shapes, style);
        if ( style.contains("trimPathEnd") || style.contains("trimPathStart") )
            add_trim(args, shapes, style);
    }

    QColor parse_color(const QString& color)
    {
        if ( !color.isEmpty() && color[0] == '#' )
        {
            if ( color.size() == 5 )
                return svg::parse_color("#" + color.mid(2) + color[1]);
            if ( color.size() == 9 )
                return svg::parse_color("#" + color.mid(3) + color.mid(1, 2));
        }

        return svg::parse_color(color);
    }

    void set_styler_style(model::Styler* styler, const QString& color)
    {
        if ( color.isEmpty() )
        {
            styler->visible.set(false);
        }
        else if ( color[0] == '@' )
        {
            auto res = get_resource(color);
            if ( res && res->element.tagName() == "gradient" )
                styler->use.set(parse_gradient(res));
        }
        else if ( color[0] == '?' )
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
        set_styler_style(stroke.get(), style.get("strokeColor", ""));
        stroke->opacity.set(percent_1(style.get("strokeAlpha", "1")));

        stroke->width.set(parse_unit(style.get("strokeWidth", "1")));

        stroke->cap.set(line_cap(style.get("strokeLineCap", "butt")));
        stroke->join.set(line_join(style.get("strokeLineJoin", "butt")));
        stroke->miter_limit.set(parse_unit(style.get("strokeMiterLimit", "4")));

        auto anim = get_animations(args.element);
        for ( const auto& kf : add_keyframes(anim.single("strokeColor")) )
            stroke->color.set_keyframe(kf.time, kf.values.color())->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("strokeAlpha")) )
            stroke->opacity.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("strokeWidth")) )
            stroke->width.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        shapes->insert(std::move(stroke));
    }

    const AnimateParser::AnimatedProperties& get_animations(const QDomElement& element)
    {
        auto name = element.attribute("name");
        return animations[name];
    }

    void add_fill(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        auto fill = std::make_unique<model::Fill>(document);
        set_styler_style(fill.get(), style.get("fillColor", ""));
        fill->opacity.set(percent_1(style.get("fillAlpha", "1")));

        if ( style.get("fillType", "") == "evenOdd" )
            fill->fill_rule.set(model::Fill::EvenOdd);

        auto anim = get_animations(args.element);
        for ( const auto& kf : add_keyframes(anim.single("fillColor")) )
            fill->color.set_keyframe(kf.time, kf.values.color())->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("fillAlpha")) )
            fill->opacity.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        shapes->insert(std::move(fill));
    }

    void add_trim(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        auto trim = std::make_unique<model::Trim>(document);

        trim->start.set(percent_1(style.get("trimPathStart", "1")));
        trim->end.set(percent_1(style.get("trimPathEnd", "1")));
        trim->offset.set(percent_1(style.get("trimPathOffset", "1")));

        auto anim = get_animations(args.element);

        for ( const auto& kf : add_keyframes(anim.single("trimPathStart")) )
            trim->start.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("trimPathEnd")) )
            trim->end.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("trimPathOffset")) )
            trim->offset.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);

        shapes->insert(std::move(trim));
    }

    model::Gradient* parse_gradient(Resource* res)
    {
        if ( res->element.tagName() != "gradient" )
            return nullptr;

        if ( res->asset )
            return res->asset->cast<model::Gradient>();

        // Load colors
        auto colors = document->assets()->add_gradient_colors();

        QGradientStops stops;
        if ( res->element.hasAttribute("startColor") )
        stops.push_back({0.0, parse_color(res->element.attribute("startColor"))});
        if ( res->element.hasAttribute("centerColor") )
            stops.push_back({0.5, parse_color(res->element.attribute("centerColor"))});
        if ( res->element.hasAttribute("endColor") )
            stops.push_back({1.0, parse_color(res->element.attribute("endColor"))});

        for ( QDomElement e : ElementRange(res->element.childNodes()) )
        {
            if ( e.tagName() == "item" )
                stops.push_back({
                    e.attribute("offset", "0").toDouble(),
                    parse_color(e.attribute("color"))
                });
        }

        colors->colors.set(stops);

        // Load gradient
        auto gradient = document->assets()->add_gradient();
        gradient->colors.set(colors);

        QString type = res->element.attribute("type", "linear");
        if ( type == "linear" )
            gradient->type.set(model::Gradient::Linear);
        else if ( type == "radial" )
            gradient->type.set(model::Gradient::Radial);
        else if ( type == "sweeo" )
            gradient->type.set(model::Gradient::Conical);

        gradient->start_point.set({
            len_attr(res->element, "startX"),
            len_attr(res->element, "startY"),
        });

        gradient->end_point.set({
            len_attr(res->element, "endX"),
            len_attr(res->element, "endY"),
        });

        // TODO center / radius


        res->asset = gradient;
        return gradient;
    }

    void parse_transform(model::Transform* trans, const ParseFuncArgs& args)
    {
        QPointF anchor = {
            len_attr(args.element, "pivotX"),
            len_attr(args.element, "pivotY"),
        };

        trans->anchor_point.set(anchor);
        trans->position.set(anchor + QPointF{
            len_attr(args.element, "translateX"),
            len_attr(args.element, "translateY"),
        });

        trans->scale.set(QVector2D(
            percent_1(args.element.attribute("scaleX", "1")),
            percent_1(args.element.attribute("scaleY", "1"))
        ));

        trans->rotation.set(args.element.attribute("rotation", "0").toDouble());

        auto anim = get_animations(args.element);

        for ( const auto& kf : add_keyframes(anim.joined({"pivotX", "pivotY", "translateX", "translateY"})) )
        {
            anchor = QPointF(kf.values[0].scalar(), kf.values[1].scalar());
            trans->anchor_point.set_keyframe(kf.time, anchor)->set_transition(kf.transition);
            QPointF pos(kf.values[2].scalar(), kf.values[3].scalar());
            trans->position.set_keyframe(kf.time, anchor + pos)->set_transition(kf.transition);
        }

        for ( const auto& kf : add_keyframes(anim.joined({"scaleX", "scaleY"})) )
        {
            QVector2D scale(kf.values[0].scalar(), kf.values[1].scalar());
            trans->scale.set_keyframe(kf.time, scale)->set_transition(kf.transition);
        }

        for ( const auto& kf : add_keyframes(anim.single("rotation")) )
            trans->rotation.set_keyframe(kf.time, kf.values.scalar())->set_transition(kf.transition);
    }

    std::unique_ptr<model::Group> parse_clip(const QDomElement& element)
    {
        auto clip = std::make_unique<model::Group>(document);
        set_name(clip.get(), element);

        QString d = element.attribute("pathData");
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

        path_animation(shapes, get_animations(element), "pathData");

        return clip;
    }

    void parseshape_group(const ParseFuncArgs& args)
    {
        std::unique_ptr<model::Group> clip;

        for ( auto e : ElementRange(args.element.elementsByTagName("clip-path")) )
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
        QString d = args.element.attribute("pathData");
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

        path_animation(paths, get_animations(args.element), "pathData");
    }

    Resource* get_resource(const QString& id)
    {
        auto iter = resources.find(id);
        if ( iter != resources.end() )
            return &iter->second;

        if ( resource_path.isRoot() || id.isEmpty() || id[0] != '@' || id.back() == '\0' )
        {
            warning(QObject::tr("Unkown resource id %1").arg(id));
            return {};
        }

        QString path = resource_path.filePath(id.mid(1) + ".xml");
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
        QString id = QString("@(internal)%1").arg(internal_resource_id);
        id.push_back('\0');
        resources[id] = {e.tagName(), e};
        return id;
    }

    model::NamedColor* color_from_theme(const QString& color)
    {
        QString norm_name;
        if ( color.contains("/") )
            norm_name = color.split("/").back();
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
    {"group",       &glaxnimate::io::avd::AvdParser::Private::parseshape_group},
    {"path",        &glaxnimate::io::avd::AvdParser::Private::parseshape_path},
};

const std::unordered_set<QString> glaxnimate::io::avd::AvdParser::Private::style_atrrs = {
    "fillColor", "fillAlpha", "fillType",
    "strokeColor", "strokeAlpha", "strokeWidth", "strokeLineCap", "strokeLineJoin", "strokeMiterLimit",
    "trimPathStart", "trimPathEnd", "trimPathOffset",
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
    {"colorForeground", "#ffffffff"},
    {"colorForegroundInverse", "#ff000000"},
    {"colorBackground", "#ff303030"},
    {"colorBackgroundFloating", "#ff424242"},
    {"colorError", "#ff7043"},
    {"opacityListDivider", "#1f000000"},
    {"textColorPrimary", "#ff000000"},
    {"textColorSecondary", "#ff000000"},
    {"textColorHighlight", "#ffffffff"},
    {"textColorHighlightInverse", "#ffffffff"},
    {"navigationBarColor", "#ff000000"},
    {"panelColorBackground", "#000"},
    {"colorPrimaryDark", "#ff000000"},
    {"colorPrimary", "#ff212121"},
    {"colorAccent", "#ff80cbc4"},
    {"tooltipForegroundColor", "#ff000000"},
    {"colorPopupBackground", "#ff303030"},
    {"colorListDivider", "#ffffffff"},
    {"textColorLink", "#ff80cbc4"},
    {"textColorLinkInverse", "#ff80cbc4"},
    {"editTextColor", "#ff000000"},
    {"windowBackground", "#ff303030"},
    {"statusBarColor", "#ff000000"},
    {"panelBackground", "#ff303030"},
    {"panelColorForeground", "#ff000000"},
    {"detailsElementBackground", "#ff303030"},
    {"actionMenuTextColor", "#ff000000"},
    {"colorEdgeEffect", "#ff212121"},
    {"colorControlNormal", "#ff000000"},
    {"colorControlActivated", "#ff80cbc4"},
    {"colorProgressBackgroundNormal", "#ff000000"},
};
