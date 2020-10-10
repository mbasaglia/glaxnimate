#include "svg_renderer.hpp"

#include <QXmlStreamWriter>

#include "model/document.hpp"
#include "model/shapes/shapes.hpp"
#include "math/math.hpp"

#include "detail.hpp"

using namespace io::svg::detail;

class io::svg::SvgRenderer::Private
{
public:
    void collect_defs(model::Document* doc)
    {
        if ( !at_start )
            return;

        at_start = false;
        writer.writeStartElement("defs");
        for ( const auto& color : doc->defs()->colors )
            write_named_color(color.get());
        for ( const auto& color : doc->defs()->gradient_colors )
            write_gradient_colors(color.get());
        for ( const auto& gradient : doc->defs()->gradients )
            write_gradient(gradient.get());
        writer.writeEndElement();

        writer.writeStartElement("sodipodi:namedview");
        write_attribute("inkscape:pagecheckerboard", "true");
        write_attribute("borderlayer", "true");
        write_attribute("bordercolor", "#666666");
        write_attribute("pagecolor", "#ffffff");
        write_attribute("inkscape:document-units", "px");
        writer.writeEndElement();
    }

    void write_composition(model::Composition* comp)
    {
        for ( const auto& lay : comp->shapes )
            write_shape(lay.get(), false);
    }

    void write_visibility_attributes(model::DocumentNode* node)
    {
        if ( !node->docnode_visible() )
            write_attribute("display", "none");
        if ( node->docnode_locked() )
            writer.writeAttribute(detail::xmlns.at("sodipodi"), "insensitive", "true");
    }

    void write_shapes(const model::ShapeListProperty& shapes)
    {
        for ( const auto& shape : shapes )
            write_shape(shape.get(), false);
    }


    QString styler_to_css(model::Styler* styler)
    {
        if ( styler->use.get() )
            return "url(#" + non_uuid_ids_map[styler->use.get()] + ")";
        return styler->color.get().name();
    }

    void write_styler_shapes(model::Styler* styler, const Style::Map& style)
    {
        if ( styler->affected().size() == 1 )
        {
            write_shape_shape(styler->affected()[0], style);
            write_visibility_attributes(styler);
            write_attribute("id", id(styler));
            return;
        }

        start_group(styler);
        write_style(style);
        write_visibility_attributes(styler);
        write_attribute("id", id(styler));

        for ( model::ShapeElement* subshape : styler->affected() )
        {
            write_shape_shape(subshape, style);
        }

        writer.writeEndElement();
    }

    void write_shape_shape(model::ShapeElement* shape, const Style::Map& style)
    {
        model::FrameTime time = shape->time();

        if ( auto rect = qobject_cast<model::Rect*>(shape) )
        {
            writer.writeEmptyElement("rect");
            write_style(style);
            QPointF c = rect->position.get_at(time);
            QSizeF s = rect->size.get_at(time);
            write_attribute("x", c.x() - s.width()/2);
            write_attribute("y", c.y() - s.height()/2);
            write_attribute("width", s.width());
            write_attribute("height", s.height());
        }
        else if ( auto ellipse = qobject_cast<model::Ellipse*>(shape) )
        {
            writer.writeEmptyElement("ellipse");
            write_style(style);
            QPointF c = ellipse->position.get_at(time);
            QSizeF s = ellipse->size.get_at(time);
            write_attribute("cx", c.x());
            write_attribute("cy", c.y());
            write_attribute("rx", s.width() / 2);
            write_attribute("ry", s.height() / 2);
        }
        else if ( auto star = qobject_cast<model::PolyStar*>(shape) )
        {
            write_bezier(shape->shapes(time), style);

            write_attribute("sodipodi:type", "star");
            write_attribute("inkscape:randomized", "0");
            write_attribute("inkscape:rounded", "0");
            int sides = star->points.get_at(time);
            write_attribute("sodipodi:sides", sides);
            write_attribute(
                "inkscape:flatsided",
                star->type.get() == model::PolyStar::Polygon ?
                "true" : "false"
            );
            QPointF c = star->position.get_at(time);
            write_attribute("sodipodi:cx", c.x());
            write_attribute("sodipodi:cy", c.y());
            write_attribute("sodipodi:r1", star->outer_radius.get_at(time));
            write_attribute("sodipodi:r2", star->inner_radius.get_at(time));
            qreal angle = math::deg2rad(star->angle.get_at(time) - 90);
            write_attribute("sodipodi:arg1", angle);
            write_attribute("sodipodi:arg2", angle + math::pi / sides);
        }
        else if ( !qobject_cast<model::Styler*>(shape) )
        {
            write_bezier(shape->shapes(time), style);
        }
    }

    void write_shape(model::ShapeElement* shape, bool force_draw)
    {
        if ( auto grp = qobject_cast<model::Group*>(shape) )
        {
            write_group_shape(grp);
        }
        else if ( auto stroke = qobject_cast<model::Stroke*>(shape) )
        {
            Style::Map style;
            style["fill"] = "none";
            style["stroke"] = styler_to_css(stroke);
            style["stroke-width"] = QString::number(stroke->width.get());
            style["stroke-opacity"] = QString::number(stroke->opacity.get());
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
                    break;
            }
            style["stroke-dasharray"] = "none";
            write_styler_shapes(stroke, style);
        }
        else if ( auto fill = qobject_cast<model::Fill*>(shape) )
        {
            Style::Map style;
            style["fill"] = styler_to_css(fill);
            style["fill-opacity"] = QString::number(fill->opacity.get());
            write_styler_shapes(fill, style);
        }
        else if ( auto img = qobject_cast<model::Image*>(shape) )
        {
            if ( img->image.get() )
            {
                writer.writeEmptyElement("image");
                write_attribute("x", 0);
                write_attribute("y", 0);
                write_attribute("width", img->image->width.get());
                write_attribute("height", img->image->height.get());
                Style::Map style;
                transform_to_style(img->transform_matrix(img->time()), style);
                write_attributes(style);
                write_attribute("xlink:href", img->image->to_url().toString());
            }
        }
        else if ( force_draw )
        {
            write_shape_shape(shape, {});
            write_visibility_attributes(shape);
            write_attribute("id", id(shape));
        }
    }

    char bezier_node_type(const math::BezierPoint& p)
    {
        switch ( p.type )
        {
            case math::BezierPointType::Smooth:
                return 's';
            case math::BezierPointType::Symmetrical:
                return 'z';
            case math::BezierPointType::Corner:
            default:
                return 'c';
        }
    }

    void write_bezier(const math::MultiBezier& shape, const Style::Map& style)
    {
        writer.writeEmptyElement("path");
        write_style(style);
        QString d;
        QString nodetypes;
        for ( const math::Bezier& b : shape.beziers() )
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
        write_attribute("d", d);
        write_attribute("sodipodi:nodetypes", nodetypes);
    }

    void write_group_shape(model::Group* group)
    {
        if ( qobject_cast<model::Layer*>(group) )
            start_layer(group);
        else
            start_group(group);
        Style::Map style;
        transform_to_style(group->transform_matrix(group->time()), style);
        style["opacity"] = group->opacity.get();
        write_style(style);
        write_visibility_attributes(group);
        write_shapes(group->shapes);
        writer.writeEndElement();
    }

    void transform_to_style(const QTransform& matr, Style::Map& style)
    {
        style["transform"] = QString("matrix(%1, %2, %3, %4, %5 %6)")
            .arg(matr.m11())
            .arg(matr.m12())
            .arg(matr.m21())
            .arg(matr.m22())
            .arg(matr.m31())
            .arg(matr.m32())
        ;
    }

    void write_style(const Style::Map& s)
    {
        QString st;
        for ( auto it : s )
        {
            st.append(it.first);
            st.append(':');
            st.append(it.second);
            st.append(';');
        }
        writer.writeAttribute("style", st);
    }

    void start_group(model::DocumentNode* node)
    {
        writer.writeStartElement("g");
        writer.writeAttribute("id", id(node));
        writer.writeAttribute("inkscape:label", node->name.get());
    }

    void start_layer(model::DocumentNode* node)
    {
        start_group(node);
        writer.writeAttribute("inkscape:groupmode", "layer");
    }

    QString id(model::ReferenceTarget* node)
    {
        return node->type_name() + "_" + node->uuid.get().toString(QUuid::Id128);
    }

    void write_attribute(const QString& name, const QString& val)
    {
        writer.writeAttribute(name, val);
    }

    void write_attributes(const Style::Map& attributes)
    {
        for ( auto it : attributes )
            write_attribute(it.first, it.second);
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

    void write_named_color(model::NamedColor* color)
    {
        writer.writeStartElement("linearGradient");
        writer.writeAttribute("osb:paint", "solid");
        QString id = pretty_id(color->name.get(), color);
        non_uuid_ids_map[color] = id;
        writer.writeAttribute("id", id);

        writer.writeEmptyElement("stop");
        writer.writeAttribute("offset", "0");
        writer.writeAttribute("style", "stop-color:" + color->color.get().name());

        writer.writeEndElement();
    }

    QString pretty_id(const QString& s, model::ReferenceTarget* node)
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
    std::enable_if_t<std::is_arithmetic_v<T>>
    write_attribute(const QString& name, T val)
    {
        writer.writeAttribute(name, QString::number(val));
    }

    void write_gradient_colors(model::GradientColors* gradient)
    {
        writer.writeStartElement("linearGradient");
        QString id = pretty_id(gradient->name.get(), gradient);
        non_uuid_ids_map[gradient] = id;
        writer.writeAttribute("id", id);

        for ( const auto& stop : gradient->colors.get() )
        {
            writer.writeEmptyElement("stop");
            write_attribute("offset", stop.first);
            writer.writeAttribute("style", "stop-color:" + stop.second.name() + ";stop-opacity:1;");
        }

        writer.writeEndElement();
    }

    void write_gradient(model::Gradient* gradient)
    {
        if ( gradient->type.get() == model::Gradient::Radial )
        {
            writer.writeStartElement("radialGradient");
            write_attribute("cx", gradient->start_point.get().x());
            write_attribute("cy", gradient->start_point.get().y());
            write_attribute("r", math::length(gradient->start_point.get() - gradient->end_point.get()));
            write_attribute("fx", gradient->highlight.get().x());
            write_attribute("fy", gradient->highlight.get().y());
        }
        else
        {
            writer.writeStartElement("linearGradient");
            write_attribute("x1", gradient->start_point.get().x());
            write_attribute("y1", gradient->start_point.get().y());
            write_attribute("x2", gradient->end_point.get().x());
            write_attribute("y2", gradient->end_point.get().y());
        }

        QString id = pretty_id(gradient->name.get(), gradient);
        non_uuid_ids_map[gradient] = id;
        writer.writeAttribute("id", id);
        writer.writeAttribute("gradientUnits", "userSpaceOnUse");

        auto it = non_uuid_ids_map.find(gradient->colors.get());
        if ( it != non_uuid_ids_map.end() )
            writer.writeAttribute("xlink:href", "#" + it->second);

        writer.writeEndElement();
    }

    QXmlStreamWriter writer;
    bool at_start = true;
    bool closed = false;
    std::set<QString> non_uuid_ids;
    std::map<model::ReferenceTarget*, QString> non_uuid_ids_map;
};


io::svg::SvgRenderer::SvgRenderer(QIODevice* device)
    : d(std::make_unique<Private>())
{
    d->writer.setDevice(device);
    d->writer.setAutoFormatting(true);
    d->writer.writeStartDocument();
    d->writer.writeStartElement("svg");
    d->writer.writeAttribute("xmlns", detail::xmlns.at("svg"));
    for ( const auto& p : detail::xmlns )
        d->writer.writeNamespace(p.second, p.first);
    d->write_style({
        {"fill", "none"},
        {"stroke", "none"}
    });
    d->write_attribute("inkscape:export-xdpi", "96");
    d->write_attribute("inkscape:export-ydpi", "96");
}

io::svg::SvgRenderer::~SvgRenderer()
{
    close();
}

void io::svg::SvgRenderer::write_document(model::Document* document)
{
    write_main(document->main());
}

void io::svg::SvgRenderer::write_composition(model::Composition* comp)
{
    d->collect_defs(comp->document());
    d->start_layer(comp);
    d->write_composition(comp);
    d->writer.writeEndElement();
}


void io::svg::SvgRenderer::write_main(model::MainComposition* comp)
{
    if ( d->at_start )
    {
        QString w  = QString::number(comp->width.get());
        QString h = QString::number(comp->height.get());
        d->writer.writeAttribute("width", w);
        d->writer.writeAttribute("height", h);
        d->writer.writeAttribute("version", "1.1");
        d->writer.writeAttribute("viewBox", QString("0 0 %1 %2").arg(w).arg(h));
        d->collect_defs(comp->document());
        write_composition(comp);
    }
    else
    {
        write_composition(comp);
    }
}

void io::svg::SvgRenderer::write_shape(model::ShapeElement* shape)
{
    d->collect_defs(shape->document());
    d->write_shape(shape, true);
}

void io::svg::SvgRenderer::write_node(model::DocumentNode* node)
{
    if ( auto mc = qobject_cast<model::MainComposition*>(node) )
        write_main(mc);
    else if ( auto co = qobject_cast<model::Composition*>(node) )
        write_composition(co);
    else if ( auto sh = qobject_cast<model::ShapeElement*>(node) )
        write_shape(sh);
}

void io::svg::SvgRenderer::close()
{
    if ( !d->closed )
    {
        d->writer.writeEndDocument();
        d->closed = true;
    }
}
