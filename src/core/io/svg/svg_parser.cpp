#include "svg_parser.hpp"

#include <unordered_set>

#include <QtXml/QDomDocument>

#include "utils/regexp.hpp"
#include "model/shapes/shapes.hpp"
#include "model/layers/layers.hpp"
#include "model/document.hpp"

#include "detail.hpp"
#include "path_parser.hpp"

using namespace io::svg::detail;

class io::svg::SvgParser::Private
{
public:
    template<class T>
    struct ItemCountRange
    {
//         using value_type = decltype(std::declval<T>().item(0));
        struct iterator
        {
            auto operator*() const { return range->dom_list.item(index); }
            iterator& operator++() { index++; return *this; }
            bool operator != (const iterator& it) const
            {
                return range != it.range || index != it.index;
            }

            const ItemCountRange* range;
            int index;
        };

        ItemCountRange(const T& dom_list) : dom_list(dom_list) {}
        iterator begin() const { return {this, 0}; }
        iterator end() const { return {this, dom_list.count()}; }

        T dom_list;
    };

    using ShapeCollection = std::vector<std::unique_ptr<model::Shape>>;

    struct ParseFuncArgs
    {
        const QDomElement& element;
        model::Layer* layer_parent;
        model::ShapeListProperty* shape_parent;
        const Style& parent_style;
        bool in_group;
    };

    void parse(bool write_to_document)
    {
        this->write_to_document = write_to_document;

        size = document->size();
        auto svg = dom.documentElement();
        dpi = attr(svg, "inkscape", "export-xdpi", "96").toDouble();
        size.setWidth(len_attr(svg, "width", size.width()));
        size.setHeight(len_attr(svg, "height", size.height()));

        /// \todo Parse defs

        model::ShapeLayer* parent_layer = parse_objects(svg);

        if ( write_to_document )
            write_document_data(parent_layer);
    }

    void write_document_data(model::ShapeLayer* parent_layer)
    {
        document->main_composition()->width.set(size.width());
        document->main_composition()->height.set(size.height());
        document->set_best_name(
            document->main_composition(),
            attr(dom.documentElement(), "sodipodi", "docname")
        );
        parent_layer->recursive_rename();
    }

    model::ShapeLayer* parse_objects(const QDomElement& svg)
    {
        model::ShapeLayer* parent_layer = add_layer<model::ShapeLayer>(nullptr);
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

        parse_children({svg, parent_layer, &parent_layer->shapes, parse_style(svg, {}), false});

        return parent_layer;
    }

    qreal len_attr(const QDomElement& e, const QString& name, qreal defval)
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

    template<class LayT>
    LayT* add_layer(model::Layer* parent)
    {
        LayT* lay = new LayT(document, composition);
        if ( write_to_document )
            document->main_composition()->layers.insert(
                std::unique_ptr<model::Layer>(lay), layer_insert++
            );
        else
            objects.emplace_back(lay);

        lay->parent.set(parent);

        return lay;
    }

    QStringList split_attr(const QDomElement& e, const QString& name)
    {
        return e.attribute(name).split(separator, QString::SkipEmptyParts);
    }

    void parse_children(const ParseFuncArgs& args)
    {
        for ( const auto& domnode : ItemCountRange(args.element.childNodes()) )
        {
            if ( domnode.isElement() )
            {
                auto child = domnode.toElement();
                parse_shape({child, args.layer_parent, args.shape_parent, args.parent_style, args.in_group});
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
            transform->set_transform_matrix(svg_transform(element.attribute("transform")));

        /// \todo adjust anchor point
    }

    std::vector<qreal> double_args(const QString& str)
    {
        auto args_s = str.splitRef(separator, QString::SkipEmptyParts);
        std::vector<qreal> args;
        args.reserve(args_s.size());
        std::transform(args_s.begin(), args_s.end(), std::back_inserter(args),
                        [](const QStringRef& s){ return s.toDouble(); });
        return args;
    }

    QTransform svg_transform(const QString& attr)
    {
        QTransform trans;

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
            node->docnode_set_visible(false);
        node->docnode_set_locked(attr(element, "sodipodi", "insensitive") == "true");
        node->set("opacity", style.get("opacity", "1").toDouble());
        node->get("transform").value<model::Transform*>();
    }

    void add_shapes(const ParseFuncArgs& args, ShapeCollection&& shapes)
    {
        Style style = parse_style(args.element, args.parent_style);
        auto group = std::make_unique<model::Group>(document);
        apply_common_style(group.get(), args.element, style);
        set_name(group.get(), args.element);

        add_style_shapes(&group->shapes, style);

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

    void add_style_shapes(model::ShapeListProperty* shapes, const Style& style)
    {
        QString paint_order = style.get("paint-order", "normal");
        if ( paint_order == "normal" )
            paint_order = "fill stroke";

        for ( const auto& sr : paint_order.splitRef(' ', QString::SkipEmptyParts) )
        {
            if ( sr == "fill" )
                add_fill(shapes, style);
            else if ( sr == "stroke" )
                add_stroke(shapes, style);
        }
    }

    void add_stroke(model::ShapeListProperty* shapes, const Style& style)
    {
        QString stroke_color = style.get("stroke", "none");
        if ( stroke_color == "none" )
            return;

        auto stroke = std::make_unique<model::Stroke>(document);
        stroke->color.set(parse_color(stroke_color, style.color));

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

        shapes->insert(std::move(stroke));
    }

    // parse attributes like opacity where it's a value in [0-1] or a percentage
    double percent_1(const QString& s)
    {
        if ( s.contains('%') )
            return s.midRef(0, s.size()-1).toDouble();
        return s.toDouble();
    }

    void add_fill(model::ShapeListProperty* shapes, const Style& style)
    {
        QString fill_color = style.get("fill", "none");
        if ( fill_color == "none" )
            return;

        auto fill = std::make_unique<model::Fill>(document);
        fill->color.set(parse_color(fill_color, style.color));

        fill->opacity.set(percent_1(style.get("fill-opacity", "1")));

        if ( style.get("fill-rule", "") == "evenodd" )
            fill->fill_rule.set(model::Fill::EvenOdd);

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
        rect->rounded.set((rx + ry) / 2);
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
        auto layer = add_layer<model::ShapeLayer>(args.layer_parent);
        parse_g_common(
            {args.element, layer, &layer->shapes, style, false},
            layer,
            layer->transform.get()
        );
    }

    void parse_g_to_shape(const ParseFuncArgs& args)
    {
        Style style = parse_style(args.element, args.parent_style);
        auto group = std::make_unique<model::Group>(document);
        parse_g_common(
            {args.element, args.layer_parent, &group->shapes, style, true},
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

    void parse_bezier_impl(const ParseFuncArgs& args, const math::MultiBezier& bez)
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

    void parseshape_line(const ParseFuncArgs& args)
    {
        math::MultiBezier bez;
        bez.move_to(QPointF(
            len_attr(args.element, "x1", 0),
            len_attr(args.element, "y1", 0)
        ));
        bez.line_to(QPointF(
            len_attr(args.element, "x2", 0),
            len_attr(args.element, "y2", 0)
        ));
        parse_bezier_impl(args, bez);
    }

    math::MultiBezier handle_poly(const ParseFuncArgs& args, bool close)
    {
        math::MultiBezier bez;

        auto coords = double_args(args.element.attribute("points", ""));
        if ( coords.size() < 4 )
            return bez;

        bez.move_to(QPointF(coords[0], coords[1]));

        for ( int i = 2; i < int(coords.size()); i+= 2 )
            bez.line_to(QPointF(coords[i], coords[i+1]));

        if ( close )
            bez.close();

        return bez;
    }

    void parseshape_polyline(const ParseFuncArgs& args)
    {
        parse_bezier_impl(args, handle_poly(args, false));
    }

    void parseshape_polygon(const ParseFuncArgs& args)
    {
        parse_bezier_impl(args, handle_poly(args, true));
    }


    void parseshape_path(const ParseFuncArgs& args)
    {
        QString d = args.element.attribute("d");
        math::MultiBezier bez = PathDParser(d.splitRef(separator)).parse();
        /// \todo sodipodi:nodetypes
        parse_bezier_impl(args, bez);
    }

    QDomDocument dom;

    qreal dpi = 96;
    QSizeF size;

    model::Document* document;
    model::Composition* composition;
    std::vector<std::unique_ptr<model::DocumentNode>> objects;
    int layer_insert = 0;

    GroupMode group_mode;
    bool write_to_document = false;
    std::function<void(const QString&)> on_warning;
    std::map<QString, void (Private::*)(const ParseFuncArgs&)> shape_parsers = {
        {"g", &Private::parseshape_g},
        {"rect", &Private::parseshape_rect},
        {"ellipse", &Private::parseshape_ellipse},
        {"circle", &Private::parseshape_circle},
        {"line", &Private::parseshape_line},
        {"polyline", &Private::parseshape_polyline},
        {"polygon", &Private::parseshape_polygon},
        {"path", &Private::parseshape_path},
        /// \todo
        /// * use
    };

    QRegularExpression unit_re{R"(([-+]?(?:[0-9]*\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?)([a-z]*))"};
    QRegularExpression separator{",\\s*|\\s+"};
    QRegularExpression transform_re{R"(([a-zA-Z]+)\s*\(([^\)]*)\))"};
};

io::svg::SvgParser::SvgParser(QIODevice* device, GroupMode group_mode, model::Document* document, model::Composition* composition)
    : d(std::make_unique<Private>())
{
    d->document = document;
    d->composition = composition;
    d->group_mode = group_mode;

    SvgParseError err;
    if ( !d->dom.setContent(device, true) )
        throw err;
}

io::svg::SvgParser::~SvgParser()
{
}


std::vector<std::unique_ptr<model::DocumentNode> > io::svg::SvgParser::parse_to_objects()
{
    d->parse(false);
    return std::move(d->objects);
}

void io::svg::SvgParser::parse_to_document(const std::function<void(const QString&)>& on_warning)
{
    d->on_warning = on_warning;
    d->parse(true);
}
