#include "svg_parser.hpp"

#include <unordered_set>

#include "utils/regexp.hpp"
#include "utils/sort_gradient.hpp"
#include "model/shapes/shapes.hpp"
#include "model/document.hpp"
#include "model/defs/named_color.hpp"

#include "path_parser.hpp"
#include "animate_parser.hpp"
#include "math/math.hpp"

using namespace io::svg::detail;

class io::svg::SvgParser::Private
{
public:
    using ShapeCollection = std::vector<std::unique_ptr<model::Shape>>;

    struct ParseFuncArgs
    {
        const QDomElement& element;
        model::ShapeListProperty* shape_parent;
        const Style& parent_style;
        bool in_group;
    };

    void parse()
    {
        size = document->size();
        auto svg = dom.documentElement();
        dpi = attr(svg, "inkscape", "export-xdpi", "96").toDouble();
        size.setWidth(len_attr(svg, "width", size.width()));
        size.setHeight(len_attr(svg, "height", size.height()));

        parse_defs();

        model::Layer* parent_layer = parse_objects(svg);

        parent_layer->name.set(
            attr(svg, "sodipodi", "docname", svg.attribute("id", parent_layer->type_name_human()))
        );

        write_document_data(svg);
    }

    void write_document_data(const QDomElement& svg)
    {
        document->main()->width.set(size.width());
        document->main()->height.set(size.height());

        document->main()->recursive_rename();

        document->main()->name.set(
            attr(svg, "sodipodi", "docname", document->main()->type_name_human())
        );

        if ( max_time > 0 )
        {
            document->main()->animation->last_frame.set(max_time);
            for ( auto lay : layers )
                lay->animation->last_frame.set(max_time);
        }
    }

    void parse_defs()
    {
        std::vector<QDomElement> later;

        for ( const auto& domnode : ItemCountRange(dom.elementsByTagName("linearGradient")) )
            parse_gradient_node(domnode, later);

        for ( const auto& domnode : ItemCountRange(dom.elementsByTagName("radialGradient")) )
            parse_gradient_node(domnode, later);

        std::vector<QDomElement> unprocessed;
        while ( !later.empty() && unprocessed.size() != later.size() )
        {
            unprocessed.clear();

            for ( const auto& element : later )
                parse_brush_style_check(element, unprocessed);

            std::swap(later, unprocessed);
        }
    }

    void parse_gradient_node(const QDomNode& domnode, std::vector<QDomElement>& later)
    {
        if ( !domnode.isElement() )
            return;

        auto gradient = domnode.toElement();
        QString id = gradient.attribute("id");
        if ( id.isEmpty() )
            return;

        if ( parse_brush_style_check(gradient, later) )
            parse_gradient_nolink(gradient, id);
    }

    bool parse_brush_style_check(const QDomElement& element, std::vector<QDomElement>& later)
    {
        QString link = attr(element, "xlink", "href");
        if ( link.isEmpty() )
            return true;

        if ( !link.startsWith("#") )
            return false;

        auto it = brush_styles.find(link);
        if ( it != brush_styles.end() )
        {
            brush_styles["#" + element.attribute("id")] = it->second;
            return false;
        }


        auto it1 = gradients.find(link);
        if ( it1 != gradients.end() )
        {
            parse_gradient(element, element.attribute("id"), it1->second);
            return false;
        }

        later.push_back(element);
        return false;
    }

    QGradientStops parse_gradient_stops(const QDomElement& gradient)
    {
        QGradientStops stops;

        for ( const auto& domnode : ItemCountRange(gradient.childNodes()) )
        {
            if ( !domnode.isElement() )
                continue;

            auto stop = domnode.toElement();

            if ( stop.tagName() != "stop" )
                continue;

            Style style = parse_style(stop, {});
            if ( !style.contains("stop-color") )
                continue;
            QColor color = parse_color(style["stop-color"], QColor());
            color.setAlphaF(color.alphaF() * style.get("stop-opacity", "1").toDouble());

            stops.push_back({stop.attribute("offset", "0").toDouble(), color});
        }

        utils::sort_gradient(stops);

        return stops;
    }

    void parse_gradient_nolink(const QDomElement& gradient, const QString& id)
    {
        QGradientStops stops = parse_gradient_stops(gradient);

        if ( stops.empty() )
            return;

        if ( stops.size() == 1 )
        {
            auto col = std::make_unique<model::NamedColor>(document);
            col->name.set(id);
            col->color.set(stops[0].second);
            brush_styles["#"+id] = col.get();
            document->defs()->colors.insert(std::move(col));
            return;
        }

        auto colors = std::make_unique<model::GradientColors>(document);
        colors->name.set(id);
        colors->colors.set(stops);
        gradients["#"+id] = colors.get();
        auto ptr = colors.get();
        document->defs()->gradient_colors.insert(std::move(colors));
        parse_gradient(gradient, id, ptr);
    }

    void parse_gradient(const QDomElement& element, const QString& id, model::GradientColors* colors)
    {
        auto gradient = std::make_unique<model::Gradient>(document);
        if ( element.tagName() == "linearGradient" )
        {
            if ( !element.hasAttribute("x1") || !element.hasAttribute("x2") ||
                 !element.hasAttribute("y1") || !element.hasAttribute("y2") )
                return;

            gradient->type.set(model::Gradient::Linear);

            gradient->start_point.set(QPointF(
                len_attr(element, "x1"),
                len_attr(element, "y1")
            ));
            gradient->end_point.set(QPointF(
                len_attr(element, "x2"),
                len_attr(element, "y2")
            ));
        }
        else if ( element.tagName() == "radialGradient" )
        {
            if ( !element.hasAttribute("cx") || !element.hasAttribute("cy") || !element.hasAttribute("r") )
                return;

            gradient->type.set(model::Gradient::Radial);

            QPointF c = QPointF(
                len_attr(element, "cx"),
                len_attr(element, "cy")
            );
            gradient->start_point.set(c);

            if ( element.hasAttribute("fx") )
                gradient->highlight.set({
                    len_attr(element, "fx"),
                    len_attr(element, "fy")
                });
            else
                gradient->highlight.set(c);

            gradient->end_point.set({c.x() + len_attr(element, "r"), c.y()});
        }
        else
        {
            return;
        }

        gradient->name.set(id);
        gradient->colors.set(colors);
        brush_styles["#"+id] = gradient.get();
        document->defs()->gradients.insert(std::move(gradient));
    }

    model::Layer* parse_objects(const QDomElement& svg)
    {
        model::Layer* parent_layer = add_layer(&document->main()->shapes);
        parent_layer->name.set(parent_layer->type_name_human());
        if ( svg.hasAttribute("viewBox") )
        {
            auto vb = split_attr(svg, "viewBox");
            if ( vb.size() == 4 )
            {
                qreal vbx = vb[0].toDouble();
                qreal vby = vb[1].toDouble();
                qreal vbw = vb[2].toDouble();
                qreal vbh = vb[3].toDouble();
                parent_layer->transform.get()->position.set(-QPointF(vbx, vby));
                parent_layer->transform.get()->scale.set(QVector2D(size.width() / vbw, size.height() / vbh));
            }
        }

        parse_children({svg, &parent_layer->shapes, parse_style(svg, {}), false});

        return parent_layer;
    }

    qreal len_attr(const QDomElement& e, const QString& name, qreal defval = 0)
    {
        if ( e.hasAttribute(name) )
            return parse_unit(e.attribute(name));
        return defval;
    }

    QString attr(const QDomElement& e, const QString& ns, const QString& name, const QString& defval = {})
    {
        if ( ns.isEmpty() )
            return e.attribute(name, defval);
        return e.attributeNS(xmlns.at(ns), name, defval);
    }

    qreal parse_unit(const QString& svg_value)
    {
        QRegularExpressionMatch match = unit_re.match(svg_value);
        if ( match.hasMatch() )
        {
            qreal value = match.captured(1).toDouble();
            QString unit = match.captured(2);
            static const constexpr qreal cmin = 2.54;
            if ( unit == "px" || unit == "" )
                return value;
            else if ( unit == "vw" )
                return value * size.width() * 0.01;
            else if ( unit == "vh" )
                return value * size.height() * 0.01;
            else if ( unit == "vmin" )
                return value * std::min(size.width(), size.height()) * 0.01;
            else if ( unit == "vmax" )
                return value * std::max(size.width(), size.height()) * 0.01;
            else if ( unit == "in" )
                return value * dpi;
            else if ( unit == "pc" )
                return value * dpi / 6;
            else if ( unit == "pt" )
                return value * dpi / 72;
            else if ( unit == "cm" )
                return value * dpi / cmin;
            else if ( unit == "mm" )
                return value * dpi / cmin / 10;
            else if ( unit == "Q" )
                return value * dpi / cmin / 40;
        }

        warning(QString("Unknown length value %1").arg(svg_value));
        return 0;
    }

    void warning(const QString& msg)
    {
        if ( on_warning )
            on_warning(msg);
    }

    model::Layer* add_layer(model::ShapeListProperty* parent)
    {
        model::Layer* lay = new model::Layer(document);
        parent->insert(std::unique_ptr<model::Layer>(lay));
        layers.push_back(lay);
        return lay;
    }

    QStringList split_attr(const QDomElement& e, const QString& name)
    {
        return e.attribute(name).split(AnimateParser::separator, QString::SkipEmptyParts);
    }

    void parse_children(const ParseFuncArgs& args)
    {
        for ( const auto& domnode : ItemCountRange(args.element.childNodes()) )
        {
            if ( domnode.isElement() )
            {
                auto child = domnode.toElement();
                parse_shape({child, args.shape_parent, args.parent_style, args.in_group});
            }
        }
    }

    Style parse_style(const QDomElement& element, const Style& parent_style)
    {
        Style style = parent_style;

        if ( element.hasAttribute("style") )
        {
            for ( const auto& item : element.attribute("style").split(';') )
            {
                auto split = item.splitRef(':');
                if ( split.size() == 2 )
                {
                    QString name = split[0].trimmed().toString();
                    if ( !name.isEmpty() && css_atrrs.count(name) )
                        style[name] = split[1].trimmed().toString();
                }
            }
        }

        for ( const auto& domnode : ItemCountRange(element.attributes()) )
        {
            auto attr = domnode.toAttr();
            if ( css_atrrs.count(attr.name()) )
                style[attr.name()] = attr.value();
        }

        for ( auto it = style.map.begin(); it != style.map.end(); )
        {
            if ( it->second == "inherit" )
            {
                QString parent = parent_style.get(it->first, "");
                if ( parent.isEmpty() || parent == "inherit" )
                {
                    it = style.map.erase(it);
                    continue;
                }
                it->second = parent;
            }

            ++it;
        }

        style.color = parse_color(style.get("color", "none"), parent_style.color);
        return style;
    }

    void parse_shape(const ParseFuncArgs& args)
    {
        auto it = shape_parsers.find(args.element.tagName());
        if ( it != shape_parsers.end() )
            (this->*it->second)(args);
    }

    template<class T>
    T* push(ShapeCollection& sc)
    {
        T* t = new T(document);
        sc.emplace_back(t);
        return t;
    }

    void populate_ids(const QDomElement& elem)
    {
        if ( elem.hasAttribute("id") )
            map_ids[elem.attribute("id")] = elem;

        for ( const auto& domnode : ItemCountRange(elem.childNodes()) )
        {
            if ( domnode.isElement() )
                populate_ids(domnode.toElement());
        }
    }

    QDomElement element_by_id(const QString& id)
    {
        // dom.elementById() doesn't work ;_;
        if ( map_ids.empty() )
            populate_ids(dom.documentElement());
        auto it = map_ids.find(id);
        if ( it == map_ids.end() )
            return {};
        return it->second;
    }

    void parse_transform(
        const QDomElement& element,
        model::DocumentNode* node,
        model::Transform* transform
    )
    {
        auto bb = node->local_bounding_rect(0);
        QPointF center = bb.center();
        if ( element.hasAttributeNS(detail::xmlns.at("inkscape"), "transform-center-x") )
        {
            qreal ix = element.attributeNS(detail::xmlns.at("inkscape"), "transform-center-x").toDouble();
            qreal iy = element.attributeNS(detail::xmlns.at("inkscape"), "transform-center-y").toDouble();
            center += QPointF(ix, iy);
        }

        if ( element.hasAttribute("transform") )
            transform->set_transform_matrix(svg_transform(
                element.attribute("transform"),
                transform->transform_matrix(transform->time())
            ));

        /// \todo adjust anchor point
    }

    std::vector<qreal> double_args(const QString& str)
    {
        auto args_s = str.splitRef(AnimateParser::separator, QString::SkipEmptyParts);
        std::vector<qreal> args;
        args.reserve(args_s.size());
        std::transform(args_s.begin(), args_s.end(), std::back_inserter(args),
                        [](const QStringRef& s){ return s.toDouble(); });
        return args;
    }

    QTransform svg_transform(const QString& attr, QTransform trans)
    {
        for ( const QRegularExpressionMatch& match : utils::regexp::find_all(transform_re, attr) )
        {
            auto args = double_args(match.captured(2));
            if ( args.empty() )
            {
                warning("Missing transformation parameters");
                continue;
            }

            QStringRef name = match.capturedRef(1);

            if ( name == "translate" )
            {
                trans.translate(args[0], args.size() > 1 ? args[1] : 0);
            }
            else if ( name == "scale" )
            {
                trans.scale(args[0] / 100, (args.size() > 1 ? args[1] : args[0]) / 100);
            }
            else if ( name == "rotate" )
            {
                qreal ang = args[0];
                if ( args.size() > 2 )
                {
                    /// \todo Set anchor point if not specified
                    qreal x = args[1];
                    qreal y = args[2];
                    trans.translate(-x, -y);
                    trans.rotate(ang);
                    trans.translate(x, y);
                }
                else
                {
                    trans.rotate(ang);
                }
            }
            else if ( name == "skewX" )
            {
                trans *= QTransform(
                    1, 0, 0,
                    qTan(args[0]), 1, 0,
                    0, 0, 1
                );
            }
            else if ( name == "skewY" )
            {
                trans *= QTransform(
                    1, qTan(args[0]), 0,
                    0, 1, 0,
                    0, 0, 1
                );
            }
            else if ( name == "matrix" )
            {
                if ( args.size() == 6 )
                {
                    trans *= QTransform(
                        args[0], args[1], 0,
                        args[2], args[3], 0,
                        args[4], args[5], 1
                    );
                }
                else
                {
                    warning("Wrong translation matrix");
                }
            }
            else
            {
                warning(QString("Unknown transformation %1").arg(name));
            }

        }
        return trans;
    }

    void apply_common_style(model::DocumentNode* node, const QDomElement& element, const Style& style)
    {
        if ( style.get("display") == "none" || style.get("visibility") == "hidden" )
            node->visible.set(false);
        node->locked.set(attr(element, "sodipodi", "insensitive") == "true");
        node->set("opacity", style.get("opacity", "1").toDouble());
        node->get("transform").value<model::Transform*>();
    }

    void add_shapes(const ParseFuncArgs& args, ShapeCollection&& shapes)
    {
        Style style = parse_style(args.element, args.parent_style);
        auto group = std::make_unique<model::Group>(document);
        apply_common_style(group.get(), args.element, style);
        set_name(group.get(), args.element);

        add_style_shapes(args, &group->shapes, style);

        for ( auto& shape : shapes )
            group->shapes.insert(std::move(shape));

        // parse_transform at the end so the bounding box isn't empty
        parse_transform(args.element, group.get(), group->transform.get());
        args.shape_parent->insert(std::move(group));
    }

    void set_name(model::DocumentNode* node, const QDomElement& element)
    {
        QString name = attr(element, "inkscape", "label");
        if ( name.isEmpty() )
            name = element.attribute("id");
        node->name.set(name);
    }

    void add_style_shapes(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        QString paint_order = style.get("paint-order", "normal");
        if ( paint_order == "normal" )
            paint_order = "fill stroke";

        for ( const auto& sr : paint_order.splitRef(' ', QString::SkipEmptyParts) )
        {
            if ( sr == "fill" )
                add_fill(args, shapes, style);
            else if ( sr == "stroke" )
                add_stroke(args, shapes, style);
        }
    }

    void add_stroke(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        QString stroke_color = style.get("stroke", "none");
        if ( stroke_color == "none" )
            return;

        auto stroke = std::make_unique<model::Stroke>(document);
        set_styler_style(stroke.get(), stroke_color, style.color);

        stroke->opacity.set(percent_1(style.get("stroke-opacity", "1")));
        stroke->width.set(parse_unit(style.get("stroke-width", "1")));

        QString linecap = style.get("stroke-linecap", "butt");
        if ( linecap == "round" )
            stroke->cap.set(model::Stroke::RoundCap);
        else if ( linecap == "butt" )
            stroke->cap.set(model::Stroke::ButtCap);
        else if ( linecap == "square" )
            stroke->cap.set(model::Stroke::SquareCap);

        QString linejoin = style.get("stroke-linejoin", "miter");
        if ( linejoin == "round" )
            stroke->join.set(model::Stroke::RoundJoin);
        else if ( linejoin == "bevel" )
            stroke->join.set(model::Stroke::BevelJoin);
        else if ( linejoin == "miter" || linejoin == "arcs" || linejoin == "miter-clip" )
            stroke->join.set(model::Stroke::MiterJoin);


        auto anim = parse_animated(args.element);
        for ( const auto& kf : add_keyframes(anim.single("stroke")) )
            stroke->color.set_keyframe(kf.time,
                QColor::fromRgbF(kf.values[0], kf.values[1], kf.values[2], kf.values[3])
            )->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("stroke-opacity")) )
            stroke->opacity.set_keyframe(kf.time, kf.values[0])->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("stroke-width")) )
            stroke->width.set_keyframe(kf.time, kf.values[0])->set_transition(kf.transition);

        shapes->insert(std::move(stroke));
    }

    // parse attributes like opacity where it's a value in [0-1] or a percentage
    double percent_1(const QString& s)
    {
        if ( s.contains('%') )
            return s.midRef(0, s.size()-1).toDouble();
        return s.toDouble();
    }

    void set_styler_style(model::Styler* styler, const QString& color_str, const QColor& current_color)
    {
        if ( !color_str.startsWith("url") )
        {
            styler->color.set(parse_color(color_str, current_color));
            return;
        }

        auto match = url_re.match(color_str);
        if ( match.hasMatch() )
        {
            QString id = match.captured(1);
            auto it = brush_styles.find(id);
            if ( it != brush_styles.end() )
            {
                styler->use.set(it->second);
                return;
            }
        }

        styler->color.set(current_color);
    }

    void add_fill(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        QString fill_color = style.get("fill", "none");
        if ( fill_color == "none" )
            return;

        auto fill = std::make_unique<model::Fill>(document);
        set_styler_style(fill.get(), fill_color, style.color);
        fill->opacity.set(percent_1(style.get("fill-opacity", "1")));

        if ( style.get("fill-rule", "") == "evenodd" )
            fill->fill_rule.set(model::Fill::EvenOdd);

        auto anim = parse_animated(args.element);
        for ( const auto& kf : add_keyframes(anim.single("fill")) )
            fill->color.set_keyframe(kf.time,
                QColor::fromRgbF(kf.values[0], kf.values[1], kf.values[2], kf.values[3])
            )->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("fill-opacity")) )
            fill->opacity.set_keyframe(kf.time, kf.values[0])->set_transition(kf.transition);

        shapes->insert(std::move(fill));
    }

    QColor parse_color(const QString& color_str, const QColor& current_color)
    {
        if ( color_str == "currentColor"  )
            return current_color;

        /// \todo test with rgba() etc
        return QColor(color_str);
    }

    void parseshape_rect(const ParseFuncArgs& args)
    {
        ShapeCollection shapes;
        auto rect = push<model::Rect>(shapes);
        qreal w = len_attr(args.element, "width", 0);
        qreal h = len_attr(args.element, "height", 0);
        rect->position.set(QPointF(
            len_attr(args.element, "x", 0) + w / 2,
            len_attr(args.element, "y", 0) + h / 2
        ));
        rect->size.set(QSizeF(w, h));
        qreal rx = len_attr(args.element, "rx", 0);
        qreal ry = len_attr(args.element, "ry", 0);
        rect->rounded.set(qMax(rx, ry));
        add_shapes(args, std::move(shapes));


        auto anim = parse_animated(args.element);
        for ( const auto& kf : add_keyframes(anim.joined({"x", "y", "width", "height"})) )
            rect->position.set_keyframe(kf.time, {
                kf.values[0][0] + kf.values[2][0] / 2,
                kf.values[1][0] + kf.values[3][0] / 2
            })->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.joined({"width", "height"})) )
            rect->size.set_keyframe(kf.time, {kf.values[0][0], kf.values[1][0]})->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.joined({"rx", "ry"})) )
            rect->rounded.set_keyframe(kf.time, qMax(kf.values[0][0], kf.values[1][0]))->set_transition(kf.transition);
    }

    void parseshape_ellipse(const ParseFuncArgs& args)
    {
        ShapeCollection shapes;
        auto ellipse = push<model::Ellipse>(shapes);
        ellipse->position.set(QPointF(
            len_attr(args.element, "cx", 0),
            len_attr(args.element, "cy", 0)
        ));
        qreal rx = len_attr(args.element, "rx", 0);
        qreal ry = len_attr(args.element, "ry", 0);
        ellipse->size.set(QSizeF(rx * 2, ry * 2));
        add_shapes(args, std::move(shapes));

        auto anim = parse_animated(args.element);
        for ( const auto& kf : add_keyframes(anim.joined({"cx", "cy"})) )
            ellipse->position.set_keyframe(kf.time, {kf.values[0][0], kf.values[1][0]})->set_transition(kf.transition);
        for ( const auto& kf : add_keyframes(anim.joined({"rx", "ry"})) )
            ellipse->size.set_keyframe(kf.time, {kf.values[0][0]*2, kf.values[1][0]*2})->set_transition(kf.transition);
    }

    void parseshape_circle(const ParseFuncArgs& args)
    {
        ShapeCollection shapes;
        auto ellipse = push<model::Ellipse>(shapes);
        ellipse->position.set(QPointF(
            len_attr(args.element, "cx", 0),
            len_attr(args.element, "cy", 0)
        ));
        qreal d = len_attr(args.element, "r", 0) * 2;
        ellipse->size.set(QSizeF(d, d));
        add_shapes(args, std::move(shapes));

        auto anim = parse_animated(args.element);
        for ( const auto& kf : add_keyframes(anim.joined({"cx", "cy"})) )
            ellipse->position.set_keyframe(kf.time, {kf.values[0][0], kf.values[1][0]})->set_transition(kf.transition);
        for ( const auto& kf : add_keyframes(anim.single({"r"})) )
            ellipse->size.set_keyframe(kf.time, {kf.values[0]*2, kf.values[0]*2})->set_transition(kf.transition);
    }

    void parseshape_g(const ParseFuncArgs& args)
    {
        switch ( group_mode )
        {
            case Groups:
                parse_g_to_shape(args);
                break;
            case Layers:
                parse_g_to_layer(args);
                break;
            case Inkscape:
                if ( args.in_group )
                    parse_g_to_shape(args);
                else if ( attr(args.element, "inkscape", "groupmode") == "layer" )
                    parse_g_to_layer(args);
                else
                    parse_g_to_shape(args);
                break;
        }
    }

    void parse_g_to_layer(const ParseFuncArgs& args)
    {
        Style style = parse_style(args.element, args.parent_style);
        auto layer = add_layer(args.shape_parent);
        parse_g_common(
            {args.element, &layer->shapes, style, false},
            layer,
            layer->transform.get()
        );
    }

    void parse_g_to_shape(const ParseFuncArgs& args)
    {
        Style style = parse_style(args.element, args.parent_style);
        auto group = std::make_unique<model::Group>(document);
        parse_g_common(
            {args.element, &group->shapes, style, true},
            group.get(),
            group->transform.get()
        );
        args.shape_parent->insert(std::move(group));
    }

    void parse_g_common(
        const ParseFuncArgs& args,
        model::DocumentNode* g_node,
        model::Transform* transform
    )
    {
        apply_common_style(g_node, args.element, args.parent_style);
        set_name(g_node, args.element);
        parse_children(args);
        parse_transform(args.element, g_node, transform);
    }

    void parse_bezier_impl(const ParseFuncArgs& args, const math::bezier::MultiBezier& bez)
    {
        if ( bez.beziers().empty() )
            return;

        ShapeCollection shapes;
        for ( const auto& bezier : bez.beziers() )
        {
            auto shape = push<model::Path>(shapes);
            shape->shape.set(bezier);
        }
        add_shapes(args, std::move(shapes));
    }


    model::Path* parse_bezier_impl_single(const ParseFuncArgs& args, const math::bezier::Bezier& bez)
    {
        ShapeCollection shapes;
        auto path = push<model::Path>(shapes);
        path->shape.set(bez);
        add_shapes(args, {std::move(shapes)});
        return path;
    }

    template<class KfCollection>
    KfCollection add_keyframes(KfCollection&& kfs)
    {
        if ( !kfs.empty() && kfs.back().time > max_time)
            max_time = kfs.back().time;

        return std::move(kfs);
    }

    detail::AnimateParser::AnimatedProperties parse_animated(const QDomElement& element)
    {
        return animate_parser.parse_animated_properties(element);
    }

    void parseshape_line(const ParseFuncArgs& args)
    {
        math::bezier::Bezier bez;
        bez.add_point(QPointF(
            len_attr(args.element, "x1", 0),
            len_attr(args.element, "y1", 0)
        ));
        bez.line_to(QPointF(
            len_attr(args.element, "x2", 0),
            len_attr(args.element, "y2", 0)
        ));
        auto path = parse_bezier_impl_single(args, bez);
        for ( const auto& kf : add_keyframes(parse_animated(args.element).joined({"x1", "y1", "x2", "y2"})) )
        {
            math::bezier::Bezier bez;
            bez.add_point({kf.values[0][0], kf.values[1][0]});
            bez.add_point({kf.values[2][0], kf.values[3][0]});
            path->shape.set_keyframe(kf.time, bez)->set_transition(kf.transition);
        }
    }

    math::bezier::Bezier build_poly(const std::vector<qreal>& coords, bool close)
    {
        math::bezier::Bezier bez;

        if ( coords.size() < 4 )
        {
            if ( !coords.empty() )
                warning("Not enough `points` for `polygon` / `polyline`");
            return bez;
        }

        bez.add_point(QPointF(coords[0], coords[1]));

        for ( int i = 2; i < int(coords.size()); i+= 2 )
            bez.line_to(QPointF(coords[i], coords[i+1]));

        if ( close )
            bez.close();

        return bez;
    }

    void handle_poly(const ParseFuncArgs& args, bool close)
    {
        auto path = parse_bezier_impl_single(args, build_poly(double_args(args.element.attribute("points", "")), close));
        if ( !path )
            return;

        for ( const auto& kf : add_keyframes(parse_animated(args.element).single("points")) )
            path->shape.set_keyframe(kf.time, build_poly(kf.values, close))->set_transition(kf.transition);

    }

    void parseshape_polyline(const ParseFuncArgs& args)
    {
        handle_poly(args, false);
    }

    void parseshape_polygon(const ParseFuncArgs& args)
    {
        handle_poly(args, true);
    }

    void parseshape_path(const ParseFuncArgs& args)
    {
        if ( parse_star(args) )
            return;
        QString d = args.element.attribute("d");
        math::bezier::MultiBezier bez = PathDParser(d.splitRef(AnimateParser::separator)).parse();
        /// \todo sodipodi:nodetypes
        parse_bezier_impl(args, bez);
    }

    bool parse_star(const ParseFuncArgs& args)
    {
        if ( attr(args.element, "sodipodi", "type") != "star" )
            return false;

        qreal randomized = attr(args.element, "inkscape", "randomized", "0").toDouble();
        if ( !qFuzzyCompare(randomized, 0.0) )
            return false;

        qreal rounded = attr(args.element, "inkscape", "rounded", "0").toDouble();
        if ( !qFuzzyCompare(rounded, 0.0) )
            return false;


        ShapeCollection shapes;
        auto shape = push<model::PolyStar>(shapes);
        shape->points.set(
            attr(args.element, "sodipodi", "sides").toInt()
        );
        auto flat = attr(args.element, "inkscape", "flatsided");
        shape->type.set(
            flat == "true" ?
            model::PolyStar::Polygon :
            model::PolyStar::Star
        );
        shape->position.set(QPointF(
            attr(args.element, "sodipodi", "cx").toDouble(),
            attr(args.element, "sodipodi", "cy").toDouble()
        ));
        shape->outer_radius.set(attr(args.element, "sodipodi", "r1").toDouble());
        shape->inner_radius.set(attr(args.element, "sodipodi", "r2").toDouble());
        shape->angle.set(
            math::rad2deg(attr(args.element, "sodipodi", "arg1").toDouble())
            +90
        );

        add_shapes(args, std::move(shapes));
        return true;
    }

    void parseshape_use(const ParseFuncArgs& args)
    {
        QString id = attr(args.element, "xlink", "href");
        if ( !id.startsWith('#') )
            return;
        id.remove(0,  1);
        QDomElement element = element_by_id(id);
        if ( element.isNull() )
            return;

        Style style = parse_style(args.element, args.parent_style);
        auto group = std::make_unique<model::Group>(document);
        apply_common_style(group.get(), args.element, style);
        set_name(group.get(), args.element);

        parse_shape({element, &group->shapes, style, true});

        group->transform.get()->position.set(
            QPointF(len_attr(args.element, "x", 0), len_attr(args.element, "y", 0))
        );
        parse_transform(args.element, group.get(), group->transform.get());
        args.shape_parent->insert(std::move(group));
    }

    void parseshape_image(const ParseFuncArgs& args)
    {
        auto bitmap = std::make_unique<model::Bitmap>(document);
        if ( !bitmap->from_url(attr(args.element, "xlink", "href")) )
        {
            QString path = attr(args.element, "sodipodi", "absref");
            if ( !bitmap->from_file(path) )
                return;
        }
        auto image = std::make_unique<model::Image>(document);
        image->image.set(document->defs()->images.insert(std::move(bitmap)));

        QTransform trans;
        if ( args.element.hasAttribute("transform") )
            trans = svg_transform(args.element.attribute("transform"), trans);
        trans.translate(
            len_attr(args.element, "x", 0),
            len_attr(args.element, "y", 0)
        );
        image->transform->set_transform_matrix(trans);

        args.shape_parent->insert(std::move(image));
    }

    QDomDocument dom;

    qreal dpi = 96;
    QSizeF size;

    model::Document* document;

    AnimateParser animate_parser;
    model::FrameTime max_time = 0;
    GroupMode group_mode;
    std::function<void(const QString&)> on_warning;
    std::unordered_map<QString, QDomElement> map_ids;
    std::unordered_map<QString, model::BrushStyle*> brush_styles;
    std::unordered_map<QString, model::GradientColors*> gradients;
    std::vector<model::Layer*> layers;

    static const std::map<QString, void (Private::*)(const ParseFuncArgs&)> shape_parsers;
    static const QRegularExpression unit_re;
    static const QRegularExpression transform_re;
    static const QRegularExpression url_re;
};

const std::map<QString, void (io::svg::SvgParser::Private::*)(const io::svg::SvgParser::Private::ParseFuncArgs&)> io::svg::SvgParser::Private::shape_parsers = {
    {"g",       &io::svg::SvgParser::Private::parseshape_g},
    {"rect",    &io::svg::SvgParser::Private::parseshape_rect},
    {"ellipse", &io::svg::SvgParser::Private::parseshape_ellipse},
    {"circle",  &io::svg::SvgParser::Private::parseshape_circle},
    {"line",    &io::svg::SvgParser::Private::parseshape_line},
    {"polyline",&io::svg::SvgParser::Private::parseshape_polyline},
    {"polygon", &io::svg::SvgParser::Private::parseshape_polygon},
    {"path",    &io::svg::SvgParser::Private::parseshape_path},
    {"use",     &io::svg::SvgParser::Private::parseshape_use},
    {"image",   &io::svg::SvgParser::Private::parseshape_image},
};
const QRegularExpression io::svg::SvgParser::Private::unit_re{R"(([-+]?(?:[0-9]*\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?)([a-z]*))"};
const QRegularExpression io::svg::SvgParser::Private::transform_re{R"(([a-zA-Z]+)\s*\(([^\)]*)\))"};
const QRegularExpression io::svg::SvgParser::Private::url_re{R"(url\s*\(\s*(#[-a-zA-Z0-9_]+)\s*\)\s*)"};
const QRegularExpression io::svg::detail::AnimateParser::separator{"\\s*,\\s*|\\s+"};
const QRegularExpression io::svg::detail::AnimateParser::clock_re{R"((?:(?:(?<hours>[0-9]+):)?(?:(?<minutes>[0-9]{2}):)?(?<seconds>[0-9]+(?:\.[0-9]+)?))|(?:(?<timecount>[0-9]+(?:\.[0-9]+)?)(?<unit>h|min|s|ms)))"};
const QRegularExpression io::svg::detail::AnimateParser::frame_separator_re{"\\s*;\\s*"};

io::svg::SvgParser::SvgParser(
    QIODevice* device,
    GroupMode group_mode,
    model::Document* document,
    const std::function<void(const QString&)>& on_warning
)
    : d(std::make_unique<Private>())
{
    d->document = document;
    d->animate_parser.fps = document->main()->fps.get();
    d->group_mode = group_mode;
    d->animate_parser.on_warning = d->on_warning = on_warning;

    SvgParseError err;
    if ( !d->dom.setContent(device, true, &err.message, &err.line, &err.column) )
        throw err;
}

io::svg::SvgParser::~SvgParser()
{
}


io::mime::DeserializedData io::svg::SvgParser::parse_to_objects()
{
    io::mime::DeserializedData data;
    data.initialize_data();
    d->document = data.document.get();
    d->parse();
    return data;
}

void io::svg::SvgParser::parse_to_document()
{
    d->parse();
}
