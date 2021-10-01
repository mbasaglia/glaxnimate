#include "svg_parser.hpp"

#include <unordered_set>

#include "utils/regexp.hpp"
#include "utils/sort_gradient.hpp"
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
#include "model/document.hpp"
#include "model/assets/named_color.hpp"

#include "path_parser.hpp"
#include "animate_parser.hpp"
#include "math/math.hpp"
#include "font_weight.hpp"
#include "css_parser.hpp"


using namespace glaxnimate::io::svg::detail;

class glaxnimate::io::svg::SvgParser::Private
{
public:
    using ShapeCollection = std::vector<std::unique_ptr<model::ShapeElement>>;

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

        if ( forced_size.isValid() )
        {
            size = forced_size;
        }
        else
        {
            size.setWidth(len_attr(svg, "width", size.width()));
            size.setHeight(len_attr(svg, "height", size.height()));
        }

        for ( const auto& p : shape_parsers )
            to_process += dom.elementsByTagName(p.first).count();

        if ( io )
            io->progress_max_changed(to_process);

        QPointF pos;
        QVector2D scale{1, 1};
        if ( svg.hasAttribute("viewBox") )
        {
            auto vb = split_attr(svg, "viewBox");
            if ( vb.size() == 4 )
            {
                qreal vbx = vb[0].toDouble();
                qreal vby = vb[1].toDouble();
                qreal vbw = vb[2].toDouble();
                qreal vbh = vb[3].toDouble();

                if ( !forced_size.isValid() )
                {
                    if ( !svg.hasAttribute("width") )
                        size.setWidth(vbw);
                    if ( !svg.hasAttribute("height") )
                        size.setHeight(vbh);
                }

                pos = -QPointF(vbx, vby);
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
        }

        for ( const auto& link_node : ItemCountRange(dom.elementsByTagName("link")) )
        {
            auto link = link_node.toElement();
            if ( link.attribute("rel") == "stylesheet" )
            {
                QString url = link.attribute("href");
                if ( !url.isEmpty() )
                    document->add_pending_asset("", url);
            }
        }

        parse_css();
        parse_defs();

        model::Layer* parent_layer = parse_objects(svg);
        parent_layer->transform.get()->position.set(-pos);
        parent_layer->transform.get()->scale.set(scale);

        parent_layer->name.set(
            attr(svg, "sodipodi", "docname", svg.attribute("id", parent_layer->type_name_human()))
        );

        write_document_data(svg);
    }

    void write_document_data(const QDomElement& svg)
    {
        document->main()->width.set(size.width());
        document->main()->height.set(size.height());

        if ( to_process < 1000 )
            document->main()->recursive_rename();

        document->main()->name.set(
            attr(svg, "sodipodi", "docname", "")
        );

        if ( max_time > 0 )
        {
            document->main()->animation->last_frame.set(max_time);
            for ( auto lay : layers )
                lay->animation->last_frame.set(max_time);
        }
    }

    void parse_css()
    {
        CssParser parser(css_blocks);

        for ( const auto& style : ItemCountRange(dom.elementsByTagName("style")) )
        {
            QString data;
            for ( const auto & child : ItemCountRange(style.childNodes()) )
            {
                if ( child.isText() || child.isCDATASection() )
                    data += child.toCharacterData().data();
            }

            if ( data.contains("@font-face") )
                document->add_pending_asset("", data.toUtf8());

            parser.parse(data);
        }

        std::stable_sort(css_blocks.begin(), css_blocks.end());
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
            auto anim = parse_animated(gradient.firstChildElement("stop"));

            for ( const auto& kf : add_keyframes(anim.single("stop-color")) )
                col->color.set_keyframe(kf.time,
                    QColor::fromRgbF(kf.values[0], kf.values[1], kf.values[2], kf.values[3])
                )->set_transition(kf.transition);

            document->assets()->colors->values.insert(std::move(col));
            return;
        }

        auto colors = std::make_unique<model::GradientColors>(document);
        colors->name.set(id);
        colors->colors.set(stops);
        gradients["#"+id] = colors.get();
        auto ptr = colors.get();
        document->assets()->gradient_colors->values.insert(std::move(colors));
        parse_gradient(gradient, id, ptr);
    }

    void parse_gradient(const QDomElement& element, const QString& id, model::GradientColors* colors)
    {
        auto gradient = std::make_unique<model::Gradient>(document);
        QTransform gradient_transform;

        if ( element.hasAttribute("gradientTransform") )
            gradient_transform = svg_transform(element.attribute("gradientTransform"), {}).transform;

        if ( element.tagName() == "linearGradient" )
        {
            if ( !element.hasAttribute("x1") || !element.hasAttribute("x2") ||
                 !element.hasAttribute("y1") || !element.hasAttribute("y2") )
                return;

            gradient->type.set(model::Gradient::Linear);

            gradient->start_point.set(gradient_transform.map(QPointF(
                len_attr(element, "x1"),
                len_attr(element, "y1")
            )));
            gradient->end_point.set(gradient_transform.map(QPointF(
                len_attr(element, "x2"),
                len_attr(element, "y2")
            )));

            auto anim = parse_animated(element);
            for ( const auto& kf : add_keyframes(anim.joined({"x1", "y1"})) )
                gradient->start_point.set_keyframe(kf.time, {kf.values[0][0], kf.values[1][0]})->set_transition(kf.transition);
            for ( const auto& kf : add_keyframes(anim.joined({"x2", "y2"})) )
                gradient->end_point.set_keyframe(kf.time, {kf.values[0][0], kf.values[1][0]})->set_transition(kf.transition);
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
            gradient->start_point.set(gradient_transform.map(c));

            if ( element.hasAttribute("fx") )
                gradient->highlight.set(gradient_transform.map(QPointF(
                    len_attr(element, "fx"),
                    len_attr(element, "fy")
                )));
            else
                gradient->highlight.set(gradient_transform.map(c));

            gradient->end_point.set(gradient_transform.map(QPointF(
                c.x() + len_attr(element, "r"), c.y()
            )));


            auto anim = parse_animated(element);
            for ( const auto& kf : add_keyframes(anim.joined({"cx", "cy"})) )
                gradient->start_point.set_keyframe(kf.time,
                    gradient_transform.map(QPointF{kf.values[0][0], kf.values[1][0]})
                )->set_transition(kf.transition);

            for ( const auto& kf : add_keyframes(anim.joined({"fx", "fy"})) )
                gradient->highlight.set_keyframe(kf.time,
                    gradient_transform.map(QPointF{kf.values[0][0], kf.values[1][0]})
                )->set_transition(kf.transition);

            for ( const auto& kf : add_keyframes(anim.joined({"cx", "cy", "r"})) )
                gradient->end_point.set_keyframe(kf.time,
                    gradient_transform.map(QPointF{kf.values[0][0] + kf.values[2][0], kf.values[1][0]})
                )->set_transition(kf.transition);

        }
        else
        {
            return;
        }

        gradient->name.set(id);
        gradient->colors.set(colors);
        brush_styles["#"+id] = gradient.get();
        document->assets()->gradients->values.insert(std::move(gradient));
    }

    model::Layer* parse_objects(const QDomElement& svg)
    {
        model::Layer* parent_layer = add_layer(&document->main()->shapes);
        parent_layer->name.set(parent_layer->type_name_human());
        Style default_style(Style::Map{
            {"fill", "black"},
        });
        parse_children({svg, &parent_layer->shapes, parse_style(svg, default_style), false});

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
            qreal mult = unit_multiplier(match.captured(2));
            if ( mult != 0 )
                return value * mult;
        }

        warning(QString("Unknown length value %1").arg(svg_value));
        return 0;
    }

    qreal unit_multiplier(const QString& unit)
    {
        static const constexpr qreal cmin = 2.54;

        if ( unit == "px" || unit == "" )
            return 1;
        else if ( unit == "vw" )
            return size.width() * 0.01;
        else if ( unit == "vh" )
            return size.height() * 0.01;
        else if ( unit == "vmin" )
            return std::min(size.width(), size.height()) * 0.01;
        else if ( unit == "vmax" )
            return std::max(size.width(), size.height()) * 0.01;
        else if ( unit == "in" )
            return dpi;
        else if ( unit == "pc" )
            return dpi / 6;
        else if ( unit == "pt" )
            return dpi / 72;
        else if ( unit == "cm" )
            return dpi / cmin;
        else if ( unit == "mm" )
            return dpi / cmin / 10;
        else if ( unit == "Q" )
            return dpi / cmin / 40;

        return 0;
    }

    qreal unit_convert(qreal val, const QString& from, const QString& to)
    {
        return val * unit_multiplier(from) / unit_multiplier(to);
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
        return e.attribute(name).split(AnimateParser::separator,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
        );
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

        auto class_names_list = element.attribute("class").split(" ",
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
        );
        std::unordered_set<QString> class_names(class_names_list.begin(), class_names_list.end());
        for ( const auto& rule : css_blocks )
        {
            if ( rule.selector.match(element, class_names) )
                rule.merge_into(style);
        }

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

        if ( !style.contains("fill") )
            style.set("fill", parent_style.get("fill"));

        style.color = parse_color(style.get("color", ""), parent_style.color);
        return style;
    }

    bool handle_mask(const ParseFuncArgs& args)
    {
        QString mask_ref;
        if ( args.element.hasAttribute("clip-path") )
            mask_ref = args.element.attribute("clip-path");
        else if ( args.element.hasAttribute("mask") )
            mask_ref = args.element.attribute("mask");

        if ( mask_ref.isEmpty() )
            return false;

        auto match = url_re.match(mask_ref);
        if ( !match.hasMatch() )
            return false;

        QString id = match.captured(1).mid(1);
        QDomElement mask_element = element_by_id(id);
        if ( mask_element.isNull() )
            return false;


        Style style = parse_style(args.element, args.parent_style);
        auto layer = std::make_unique<model::Layer>(document);
        apply_common_style(layer.get(), args.element, style);
        set_name(layer.get(), args.element);
        layer->mask->mask.set(true);

        QDomElement element = args.element;

        QDomElement trans_copy = dom.createElement("g");
        trans_copy.setAttribute("style", element.attribute("style"));
        element.removeAttribute("style");
        trans_copy.setAttribute("transform", element.attribute("transform"));
        element.removeAttribute("transform");

        for ( const auto& attr : detail::css_atrrs )
            element.removeAttribute(attr);

        parse_g_to_layer({
            mask_element,
            &layer->shapes,
            style,
            false
        });

        parse_shape_impl({
            element,
            &layer->shapes,
            style,
            false
        });

        parse_transform(trans_copy, layer.get(), layer->transform.get());

        args.shape_parent->insert(std::move(layer));
        return true;
    }

    void parse_shape_impl(const ParseFuncArgs& args)
    {
        auto it = shape_parsers.find(args.element.tagName());
        if ( it != shape_parsers.end() )
        {
            mark_progress();
            (this->*it->second)(args);
        }
    }

    void parse_shape(const ParseFuncArgs& args)
    {
        if ( handle_mask(args) )
            return;

        parse_shape_impl(args);
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
        model::VisualNode* node,
        model::Transform* transform
    )
    {
        auto bb = node->local_bounding_rect(0);
        QPointF center = bb.center();
        if ( element.hasAttributeNS(detail::xmlns.at("inkscape"), "transform-center-x") )
        {
            qreal ix = element.attributeNS(detail::xmlns.at("inkscape"), "transform-center-x").toDouble();
            qreal iy = -element.attributeNS(detail::xmlns.at("inkscape"), "transform-center-y").toDouble();
            center += QPointF(ix, iy);
        }

        bool anchor_from_rotate = false;

        if ( element.hasAttribute("transform") )
        {
            auto trans = svg_transform(
                element.attribute("transform"),
                transform->transform_matrix(transform->time())
            );
            transform->set_transform_matrix(trans.transform);
            anchor_from_rotate = trans.anchor_set;
            if ( trans.anchor_set )
                center = trans.anchor;

        }

        /// Adjust anchor point
        QPointF delta_pos;
        if ( anchor_from_rotate )
        {
            transform->anchor_point.set(center);
            delta_pos = center;
        }
        else
        {
            auto matrix = transform->transform_matrix(transform->time());
            QPointF p1 = matrix.map(QPointF(0, 0));
            transform->anchor_point.set(center);
            matrix = transform->transform_matrix(transform->time());
            QPointF p2 = matrix.map(QPointF(0, 0));
            delta_pos = p1 - p2;
        }
        transform->position.set(transform->position.get() + delta_pos);

        auto anim = animate_parser.parse_animated_transform(element);

        for ( const auto& kf : add_keyframes(anim.single("translate")) )
            transform->position.set_keyframe(kf.time, QPointF{kf.values[0], kf.values[1]} + delta_pos)->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("scale")) )
            transform->scale.set_keyframe(kf.time, QVector2D(kf.values[0], kf.values[1]))->set_transition(kf.transition);

        for ( const auto& kf : add_keyframes(anim.single("rotate")) )
        {
            transform->rotation.set_keyframe(kf.time, kf.values[0])->set_transition(kf.transition);
            if ( kf.values.size() == 3 )
            {
                QPointF p = {kf.values[1], kf.values[2]};
                transform->anchor_point.set_keyframe(kf.time, p)->set_transition(kf.transition);
                transform->position.set_keyframe(kf.time, p)->set_transition(kf.transition);
            }
        }
    }

    std::vector<qreal> double_args(const QString& str)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        auto args_s = str.splitRef(AnimateParser::separator, Qt::SkipEmptyParts);
#else
        auto args_s = str.splitRef(AnimateParser::separator, QString::SkipEmptyParts);
#endif
        std::vector<qreal> args;
        args.reserve(args_s.size());
        std::transform(args_s.begin(), args_s.end(), std::back_inserter(args),
                        [](const QStringRef& s){ return s.toDouble(); });
        return args;
    }

    struct ParsedTransformInfo
    {
        QTransform transform;
        QPointF anchor = {};
        bool anchor_set = false;
    };

    ParsedTransformInfo svg_transform(const QString& attr, const QTransform& trans)
    {
        ParsedTransformInfo info{trans};
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
                info.transform.translate(args[0], args.size() > 1 ? args[1] : 0);
            }
            else if ( name == "scale" )
            {
                info.transform.scale(args[0], (args.size() > 1 ? args[1] : args[0]));
            }
            else if ( name == "rotate" )
            {
                qreal ang = args[0];
                if ( args.size() > 2 )
                {
                    qreal x = args[1];
                    qreal y = args[2];
                    info.anchor = {x, y};
                    info.anchor_set = true;
//                     info.transform.translate(-x, -y);
                    info.transform.rotate(ang);
//                     info.transform.translate(x, y);
                }
                else
                {
                    info.transform.rotate(ang);
                }
            }
            else if ( name == "skewX" )
            {
                info.transform *= QTransform(
                    1, 0, 0,
                    qTan(args[0]), 1, 0,
                    0, 0, 1
                );
            }
            else if ( name == "skewY" )
            {
                info.transform *= QTransform(
                    1, qTan(args[0]), 0,
                    0, 1, 0,
                    0, 0, 1
                );
            }
            else if ( name == "matrix" )
            {
                if ( args.size() == 6 )
                {
                    info.transform *= QTransform(
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
        return info;
    }

    static qreal opacity_value(const QString& v)
    {
        if ( v.isEmpty() )
            return 1;

        if ( v.back() == '%' )
            return v.mid(0, v.size()-1).toDouble() / 100;

        return v.toDouble();
    }

    void apply_common_style(model::VisualNode* node, const QDomElement& element, const Style& style)
    {
        if ( style.get("display") == "none" || style.get("visibility") == "hidden" )
            node->visible.set(false);
        node->locked.set(attr(element, "sodipodi", "insensitive") == "true");
        node->set("opacity", opacity_value(style.get("opacity", "1")));
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

        for ( const auto& sr : paint_order.splitRef(' ',
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
        ) )
        {
            if ( sr == "fill" )
                add_fill(args, shapes, style);
            else if ( sr == "stroke" )
                add_stroke(args, shapes, style);
        }
    }

    void add_stroke(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        QString stroke_color = style.get("stroke", "transparent");
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
        QString fill_color = style.get("fill", "");

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

        if ( fill_color == "none" )
            fill->visible.set(false);

        shapes->insert(std::move(fill));
    }

    QColor parse_color(const QString& color_str, const QColor& current_color)
    {
        if ( color_str.isEmpty() || color_str == "currentColor" )
            return current_color;

        return glaxnimate::io::svg::parse_color(color_str);
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

        add_shapes(args, std::move(shapes));
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

        auto anim = parse_animated(args.element);
        for ( const auto& kf : add_keyframes(anim.joined({"cx", "cy"})) )
            ellipse->position.set_keyframe(kf.time, {kf.values[0][0], kf.values[1][0]})->set_transition(kf.transition);
        for ( const auto& kf : add_keyframes(anim.joined({"rx", "ry"})) )
            ellipse->size.set_keyframe(kf.time, {kf.values[0][0]*2, kf.values[1][0]*2})->set_transition(kf.transition);

        add_shapes(args, std::move(shapes));
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

        auto anim = parse_animated(args.element);
        for ( const auto& kf : add_keyframes(anim.joined({"cx", "cy"})) )
            ellipse->position.set_keyframe(kf.time, {kf.values[0][0], kf.values[1][0]})->set_transition(kf.transition);
        for ( const auto& kf : add_keyframes(anim.single({"r"})) )
            ellipse->size.set_keyframe(kf.time, {kf.values[0]*2, kf.values[0]*2})->set_transition(kf.transition);

        add_shapes(args, std::move(shapes));
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
        model::VisualNode* g_node,
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
        math::bezier::MultiBezier bez = PathDParser(d).parse();
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
        image->image.set(document->assets()->images->values.insert(std::move(bitmap)));

        QTransform trans;
        if ( args.element.hasAttribute("transform") )
            trans = svg_transform(args.element.attribute("transform"), trans).transform;
        trans.translate(
            len_attr(args.element, "x", 0),
            len_attr(args.element, "y", 0)
        );
        image->transform->set_transform_matrix(trans);

        args.shape_parent->insert(std::move(image));
    }

    struct TextStyle
    {
        QString family = "sans-serif";
        int weight = QFont::Normal;
        QFont::Style style = QFont::StyleNormal;
        qreal line_spacing = 0;
        qreal size = 64;
        bool keep_space = false;
        QPointF pos;
    };

    TextStyle parse_text_style(const ParseFuncArgs& args, const TextStyle& parent)
    {
        TextStyle out = parent;

        Style style = parse_style(args.element, args.parent_style);

        if ( style.contains("font-family") )
            out.family = style["font-family"];

        if ( style.contains("font-style") )
        {
            QString slant = style["font-style"];
            if ( slant == "normal" ) out.style = QFont::StyleNormal;
            else if ( slant == "italic" ) out.style = QFont::StyleItalic;
            else if ( slant == "oblique" ) out.style = QFont::StyleOblique;
        }

        if ( style.contains("font-size") )
        {
            QString size = style["font-size"];
            static const std::map<QString, int> size_names = {
                {{"xx-small"}, {8}},
                {{"x-small"}, {16}},
                {{"small"}, {32}},
                {{"medium"}, {64}},
                {{"large"}, {128}},
                {{"x-large"}, {256}},
                {{"xx-large"}, {512}},
            };
            if ( size == "smaller" )
                out.size /= 2;
            else if ( size == "larger" )
                out.size *= 2;
            else if ( size_names.count(size) )
                out.size = size_names.at(size);
            else
                out.size = parse_unit(size);
        }

        if ( style.contains("font-weight") )
        {
            QString weight = style["font-weight"];
            if ( weight == "bold" )
                out.weight = 700;
            else if ( weight == "normal" )
                out.weight = 400;
            else if ( weight == "bolder" )
                out.weight = qMin(1000, out.weight + 100);
            else if ( weight == "lighter")
                out.weight = qMax(1, out.weight - 100);
            else
                out.weight = weight.toInt();
        }

        if ( style.contains("line-height") )
            out.line_spacing = parse_unit(style["line-height"]);


        if ( args.element.hasAttribute("xml:space") )
            out.keep_space = args.element.attribute("xml:space") == "preserve";

        if ( args.element.hasAttribute("x") )
            out.pos.setX(len_attr(args.element, "x", 0));
        if ( args.element.hasAttribute("y") )
            out.pos.setY(len_attr(args.element, "y", 0));

        return out;
    }

    QString trim_text(const QString& text)
    {
        QString trimmed = text.simplified();
        if ( !text.isEmpty() && text.back().isSpace() )
            trimmed += ' ';
        return trimmed;
    }

    void apply_text_style(model::Font* font, const TextStyle& style)
    {
        font->family.set(style.family);
        font->size.set(unit_convert(style.size, "px", "pt"));
        QFont qfont;
        qfont.setFamily(style.family);
        qfont.setWeight(WeightConverter::convert(style.weight, WeightConverter::css, WeightConverter::qt));
        qfont.setStyle(style.style);
        QFontDatabase db;
        QString style_string = db.styleString(qfont);
        font->style.set(style_string);
    }

    QPointF parse_text_element(const ParseFuncArgs& args, const TextStyle& parent_style)
    {
        TextStyle style = parse_text_style(args, parent_style);
        Style css_style = parse_style(args.element, args.parent_style);

        auto anim = parse_animated(args.element);

        model::TextShape* last = nullptr;

        QPointF offset;
        QPointF pos = style.pos;
        QString text;
        for ( const auto & child : ItemCountRange(args.element.childNodes()) )
        {
            ParseFuncArgs child_args = {child.toElement(), args.shape_parent, css_style, args.in_group};
            if ( child.isElement() )
            {
                last = nullptr;
                style.pos = pos + offset;
                offset = parse_text_element(child_args, style);
            }
            else if ( child.isText() || child.isCDATASection() )
            {
                text += child.toCharacterData().data();

                if ( !last )
                {
                    ShapeCollection shapes;
                    last = push<model::TextShape>(shapes);

                    last->position.set(pos + offset);
                    apply_text_style(last->font.get(), style);

                    for ( const auto& kf : add_keyframes(anim.joined({"x", "y"})) )
                    {
                        last->position.set_keyframe(
                            kf.time,
                            offset + QPointF(kf.values[0][0], kf.values[1][0])
                        )->set_transition(kf.transition);
                    }

                    add_shapes(child_args, std::move(shapes));
                }

                last->text.set(style.keep_space ? text : trim_text(text));

                offset = last->offset_to_next_character();
            }
        }

        return offset;
    }

    void parseshape_text(const ParseFuncArgs& args)
    {
        parse_text_element(args, {});
    }

    void mark_progress()
    {
        processed++;
        if ( io && processed % 10 == 0 )
            io->progress(processed);
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
    std::vector<CssStyleBlock> css_blocks;

    int to_process = 0;
    int processed = 0;
    ImportExport* io = nullptr;
    QSize forced_size;

    static const std::map<QString, void (Private::*)(const ParseFuncArgs&)> shape_parsers;
    static const QRegularExpression unit_re;
    static const QRegularExpression transform_re;
    static const QRegularExpression url_re;
};

const std::map<QString, void (glaxnimate::io::svg::SvgParser::Private::*)(const glaxnimate::io::svg::SvgParser::Private::ParseFuncArgs&)> glaxnimate::io::svg::SvgParser::Private::shape_parsers = {
    {"g",       &glaxnimate::io::svg::SvgParser::Private::parseshape_g},
    {"rect",    &glaxnimate::io::svg::SvgParser::Private::parseshape_rect},
    {"ellipse", &glaxnimate::io::svg::SvgParser::Private::parseshape_ellipse},
    {"circle",  &glaxnimate::io::svg::SvgParser::Private::parseshape_circle},
    {"line",    &glaxnimate::io::svg::SvgParser::Private::parseshape_line},
    {"polyline",&glaxnimate::io::svg::SvgParser::Private::parseshape_polyline},
    {"polygon", &glaxnimate::io::svg::SvgParser::Private::parseshape_polygon},
    {"path",    &glaxnimate::io::svg::SvgParser::Private::parseshape_path},
    {"use",     &glaxnimate::io::svg::SvgParser::Private::parseshape_use},
    {"image",   &glaxnimate::io::svg::SvgParser::Private::parseshape_image},
    {"text",    &glaxnimate::io::svg::SvgParser::Private::parseshape_text},
};
const QRegularExpression glaxnimate::io::svg::SvgParser::Private::unit_re{R"(([-+]?(?:[0-9]*\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?)([a-z]*))"};
const QRegularExpression glaxnimate::io::svg::SvgParser::Private::transform_re{R"(([a-zA-Z]+)\s*\(([^\)]*)\))"};
const QRegularExpression glaxnimate::io::svg::SvgParser::Private::url_re{R"(url\s*\(\s*(#[-a-zA-Z0-9_]+)\s*\)\s*)"};
const QRegularExpression glaxnimate::io::svg::detail::AnimateParser::separator{"\\s*,\\s*|\\s+"};
const QRegularExpression glaxnimate::io::svg::detail::AnimateParser::clock_re{R"((?:(?:(?<hours>[0-9]+):)?(?:(?<minutes>[0-9]{2}):)?(?<seconds>[0-9]+(?:\.[0-9]+)?))|(?:(?<timecount>[0-9]+(?:\.[0-9]+)?)(?<unit>h|min|s|ms)))"};
const QRegularExpression glaxnimate::io::svg::detail::AnimateParser::frame_separator_re{"\\s*;\\s*"};

glaxnimate::io::svg::SvgParser::SvgParser(
    QIODevice* device,
    GroupMode group_mode,
    model::Document* document,
    const std::function<void(const QString&)>& on_warning,
    ImportExport* io,
    QSize forced_size
)
    : d(std::make_unique<Private>())
{
    d->document = document;
    d->animate_parser.fps = d->document ? d->document->main()->fps.get() : 60;
    d->group_mode = group_mode;
    d->animate_parser.on_warning = d->on_warning = on_warning;
    d->io = io;
    d->forced_size = forced_size;

    SvgParseError err;
    if ( !d->dom.setContent(device, true, &err.message, &err.line, &err.column) )
        throw err;
}

glaxnimate::io::svg::SvgParser::~SvgParser()
{
}


glaxnimate::io::mime::DeserializedData glaxnimate::io::svg::SvgParser::parse_to_objects()
{
    glaxnimate::io::mime::DeserializedData data;
    data.initialize_data();
    d->document = data.document.get();
    d->parse();
    return data;
}

void glaxnimate::io::svg::SvgParser::parse_to_document()
{
    d->parse();
}

QColor glaxnimate::io::svg::parse_color(const QString& string)
{
    if ( string.isEmpty() )
        return {};

    // #fff #112233
    if ( string[0] == '#' )
        return QColor(string);

    // transparent
    if ( string == "transparent" || string == "none" )
        return QColor(0, 0, 0, 0);

    QRegularExpressionMatch match;

    // rgba(123, 123, 123, 0.7)
    static QRegularExpression rgba{R"(^rgba\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9.eE]+)\s*\)$)"};
    match = rgba.match(string);
    if ( match.hasMatch() )
        return QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt(), match.captured(4).toDouble() * 255);

    // rgb(123, 123, 123)
    static QRegularExpression rgb{R"(^rgb\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)$)"};
    match = rgb.match(string);
    if ( match.hasMatch() )
        return QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());

    // rgba(60%, 30%, 20%, 0.7)
    static QRegularExpression rgba_pc{R"(^rgba\s*\(\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)\s*\)$)"};
    match = rgba_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromRgbF(match.captured(1).toDouble() / 100, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100, match.captured(4).toDouble());

    // rgb(60%, 30%, 20%)
    static QRegularExpression rgb_pc{R"(^rgb\s*\(\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*\)$)"};
    match = rgb_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromRgbF(match.captured(1).toDouble() / 100, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100);

    // hsl(60, 30%, 20%)
    static QRegularExpression hsl{R"(^hsl\s*\(\s*([0-9.eE]+)\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*\)$)"};
    match = rgb_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromHslF(match.captured(1).toDouble() / 360, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100);

    // hsla(60, 30%, 20%, 0.7)
    static QRegularExpression hsla{R"(^hsla\s*\(\s*([0-9.eE]+)\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)\s*\)$)"};
    match = rgb_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromHslF(match.captured(1).toDouble() / 360, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100, match.captured(4).toDouble());

    // red
    return QColor(string);
}
