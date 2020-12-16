#include "svg_renderer.hpp"

#include "model/document.hpp"
#include "model/shapes/shapes.hpp"
#include "model/animation/join_animatables.hpp"
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

        fps = doc->main()->fps.get();
        ip = doc->main()->animation->first_frame.get();
        op = doc->main()->animation->last_frame.get();
        if ( ip >= op )
            animated = NotAnimated;

        at_start = false;
        defs = element(svg, "defs");
        for ( const auto& color : doc->defs()->colors )
            write_named_color(defs, color.get());
        for ( const auto& color : doc->defs()->gradient_colors )
            write_gradient_colors(defs, color.get());
        for ( const auto& gradient : doc->defs()->gradients )
            write_gradient(defs, gradient.get());

        auto view = element(svg, "sodipodi:namedview");
        view.setAttribute("inkscape:pagecheckerboard", "true");
        view.setAttribute("borderlayer", "true");
        view.setAttribute("bordercolor", "#666666");
        view.setAttribute("pagecolor", "#ffffff");
        view.setAttribute("inkscape:document-units", "px");
    }

    QDomElement element(QDomNode& parent, const char* tag)
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

    void write_visibility_attributes(QDomElement& parent, model::DocumentNode* node)
    {
        if ( !node->visible.get() )
            parent.setAttribute("display", "none");
        if ( node->locked.get() )
            parent.setAttribute("sodipodi:insensitive", "true");
    }

    void write_shapes(QDomElement& parent, const model::ShapeListProperty& shapes)
    {
        for ( const auto& shape : shapes )
            write_shape(parent, shape.get(), false);
    }


    QString styler_to_css(model::Styler* styler)
    {
        if ( styler->use.get() )
            return "url(#" + non_uuid_ids_map[styler->use.get()] + ")";
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

    struct AnimationData
    {
        struct Attribute
        {
            QString attribute;
            QStringList values = {};
        };

        AnimationData(SvgRenderer::Private* parent, const std::vector<QString>& attrs, int n_keyframes)
            : parent(parent)
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
            if ( key_times.empty() && time > parent->ip )
            {
                key_times.push_back("0");
                key_splines.push_back("0 0 1 1");
                add_values(vals);
            }

            key_times.push_back(QString::number(math::unlerp(parent->ip, parent->op, time), 'f'));
            key_splines.push_back(key_spline(trans));

            for ( std::size_t i = 0; i != attributes.size(); i++ )
                attributes[i].values.push_back(vals[i]);

            last = time;
        }

        void add_dom(QDomElement& element, const char* tag = "animate", const QString& type = {})
        {
            if ( last < parent->op )
            {
                key_times.push_back("1");
                for ( auto& attr : attributes )
                    attr.values.push_back(attr.values.back());
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
                animation.setAttribute("begin", parent->clock(parent->ip));
                animation.setAttribute("dur", parent->clock(parent->op-parent->ip));
                animation.setAttribute("attributeName", data.attribute);
                animation.setAttribute("calcMode", "spline");
                animation.setAttribute("values", data.values.join("; "));
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
            int kf_count = property->keyframe_count();
            if ( kf_count < 2 )
                return;

            AnimationData data(this, {attr}, property->keyframe_count());

            for ( int i = 0; i < kf_count; i++ )
            {
                auto kf = property->keyframe(i);
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
        std::vector<model::AnimatableBase*> properties,
        const std::vector<QString>& attrs,
        const Callback& callback
    )
    {
        auto jflags = animated == NotAnimated ? model::JoinAnimatables::NoKeyframes : model::JoinAnimatables::Normal;
        model::JoinAnimatables j(std::move(properties), jflags);

        {
            auto vals = callback(j.current_value());
            for ( std::size_t i = 0; i != attrs.size(); i++ )
                element.setAttribute(attrs[i], vals[i]);
        }

        if ( j.animated() && animated )
        {
            AnimationData data(this, attrs, j.keyframes().size());

            for ( const auto& kf : j )
                data.add_keyframe(time_to_global(kf.time), callback(kf.values), kf.transition());

            data.add_dom(element);
        }
    }

    static std::vector<QString> callback_point(const std::vector<QVariant>& values)
    {
        QPointF c = values[0].toPointF();
        return std::vector<QString>{
            QString::number(c.x()),
            QString::number(c.y())
        };
    }

    void write_shape_shape(QDomElement& parent, model::ShapeElement* shape, const Style::Map& style)
    {
        model::FrameTime time = shape->time();

        if ( auto rect = qobject_cast<model::Rect*>(shape) )
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
        else if ( auto ellipse = qobject_cast<model::Ellipse*>(shape) )
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
        else if ( auto star = qobject_cast<model::PolyStar*>(shape) )
        {
            auto e = write_bezier(parent, shape, style);

            set_attribute(e, "sodipodi:type", "star");
            set_attribute(e, "inkscape:randomized", "0");
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

    void write_shape(QDomElement& parent, model::ShapeElement* shape, bool force_draw)
    {
        if ( auto grp = qobject_cast<model::Group*>(shape) )
        {
            write_group_shape(parent, grp);
        }
        else if ( auto stroke = qobject_cast<model::Stroke*>(shape) )
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
        else if ( auto fill = qobject_cast<model::Fill*>(shape) )
        {
            Style::Map style;
            if ( !animated )
            {
                style["fill"] = styler_to_css(fill);
                style["fill-opacity"] = QString::number(fill->opacity.get());
            }
            QDomElement g = write_styler_shapes(parent, fill, style);
            if ( animated )
                write_styler_attrs(g, fill, "fill");
        }
        else if ( auto img = qobject_cast<model::Image*>(shape) )
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
        else if ( auto layer = qobject_cast<model::PreCompLayer*>(shape) )
        {
            if ( layer->composition.get() )
            {
                timing.push_back(layer->timing.get());
                auto clip = element(defs, "clipPath834");
                set_attribute(clip, "id", "clip_" + id(shape));
                set_attribute(clip, "clipPathUnits", "userSpaceOnUse");
                auto clip_rect = element(clip, "rect");
                set_attribute(clip_rect, "x", "0");
                set_attribute(clip_rect, "y", "0");
                set_attribute(clip_rect, "width", layer->size.get().width());
                set_attribute(clip_rect, "height", layer->size.get().height());

                auto e = start_layer(parent, layer);
                transform_to_attr(e, layer->transform.get());
                write_visibility_attributes(parent, shape);
                write_composition(e, layer->composition.get());
                timing.pop_back();
            }
        }
        else if ( force_draw )
        {
            write_shape_shape(parent, shape, {});
            write_visibility_attributes(parent, shape);
            set_attribute(parent, "id", id(shape));
        }
    }

    char bezier_node_type(const math::bezier::Point& p)
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

    std::pair<QString, QString> path_data(const math::bezier::MultiBezier& shape)
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
            std::vector<model::AnimatableBase*> props;
            for ( auto prop : shape->properties() )
            {
                if ( prop->traits().flags & model::PropertyTraits::Animated )
                    props.push_back(static_cast<model::AnimatableBase*>(prop));
            }

            model::JoinAnimatables j(std::move(props), model::JoinAnimatables::NoValues);

            if ( j.animated() )
            {
                AnimationData data(this, {"d"}, j.keyframes().size());

                for ( const auto& kf : j )
                    data.add_keyframe(time_to_global(kf.time), {path_data(shape->shapes(kf.time)).first}, kf.transition());

                data.add_dom(path);
            }
        }
        return path;
    }

    QDomElement start_layer_recurse_parents(QDomElement parent, model::Layer* ancestor, model::Layer* descendant)
    {
        QDomElement g = element(parent, "g");
        g.setAttribute("id", id(descendant) + "_" + id(ancestor));
        g.setAttribute("inkscape:label", QObject::tr("%1 (%2)").arg(descendant->object_name()).arg(ancestor->object_name()));
        g.setAttribute("inkscape:groupmode", "layer");
        transform_to_attr(g, ancestor->transform.get());
        return g;
    }

    QDomElement recurse_parents(QDomElement& parent, model::Layer* ancestor, model::Layer* descendant)
    {
        if ( !ancestor->parent.get() )
            return start_layer_recurse_parents(parent, ancestor, descendant);
        return start_layer_recurse_parents(recurse_parents(parent, ancestor->parent.get(), descendant), ancestor, descendant);
    }

    void write_group_shape(QDomElement& parent, model::Group* group)
    {
        QDomElement g;
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


            if ( animated && layer->visible.get() )
            {
                auto* lay_range = layer->animation.get();
                auto* doc_range = layer->document()->main()->animation.get();
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

                    times += clock(ip) + ";";

                    if ( has_start )
                    {
                        vals += "none;inline;";
                        times += clock(lay_range->first_frame.get()) + ";";
                    }
                    else
                    {
                        vals += "inline;";
                    }

                    if ( has_end )
                    {
                        vals += "none;";
                        times += clock(lay_range->last_frame.get()) + ";";
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

        transform_to_attr(g, group->transform.get());
        set_attribute(g, "opacity", group->opacity.get());
        write_visibility_attributes(g, group);
        write_shapes(g, group->shapes);
    }

    template<class PropT, class Callback>
    QDomElement transform_property(QDomElement& e, const char* name, PropT* prop, const Callback& callback)
    {
        model::JoinAnimatables j({prop}, model::JoinAnimatables::NoValues);

        auto parent = e.parentNode();
        QDomElement g = dom.createElement("g");
        parent.insertBefore(g, e);
        parent.removeChild(e);
        g.appendChild(e);

        if ( j.animated() )
        {
            AnimationData data(this, {"transform"}, j.keyframes().size());

            for ( const auto& kf : j )
                data.add_keyframe(time_to_global(kf.time), {callback(prop->get_at(kf.time))}, kf.transition());
            data.add_dom(g, "animateTransform", name);
        }

        g.setAttribute("transform", QString("%1(%2)").arg(name).arg(callback(prop->get())));
        return g;
    }

    void transform_to_attr(QDomElement& parent, model::Transform* transf)
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
            subject = transform_property(subject, "translate", &transf->position, [](const QPointF& val){
                return QString("%1 %2").arg(val.x()).arg(val.y());
            });
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

    QString id(model::ReferenceTarget* node)
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
                AnimationData data(this, {"offset", "stop-color"}, gradient->colors.keyframe_count());
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
        if ( gradient->type.get() == model::Gradient::Radial )
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
    std::map<model::ReferenceTarget*, QString> non_uuid_ids_map;
    AnimationType animated;
    QDomElement svg;
    QDomElement defs;
};


io::svg::SvgRenderer::SvgRenderer(AnimationType animated)
    : d(std::make_unique<Private>())
{
    d->animated = animated;
    d->svg = d->dom.createElement("svg");
    d->dom.appendChild(d->svg);
    d->svg.setAttribute("xmlns", detail::xmlns.at("svg"));
    for ( const auto& p : detail::xmlns )
        d->svg.setAttribute("xmlns:" + p.first, p.second);

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

void io::svg::SvgRenderer::write_document(model::Document* document)
{
    write_main(document->main());
}

void io::svg::SvgRenderer::write_composition(model::Composition* comp)
{
    d->collect_defs(comp->document());
    auto g = d->start_layer(d->svg, comp);
    d->write_composition(g, comp);
}


void io::svg::SvgRenderer::write_main(model::MainComposition* comp)
{
    if ( d->at_start )
    {
        QString w  = QString::number(comp->width.get());
        QString h = QString::number(comp->height.get());
        d->svg.setAttribute("width", w);
        d->svg.setAttribute("height", h);
        d->svg.setAttribute("viewBox", QString("0 0 %1 %2").arg(w).arg(h));
        d->collect_defs(comp->document());
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
    d->collect_defs(shape->document());
    d->write_shape(d->svg, shape, true);
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

QDomDocument io::svg::SvgRenderer::dom() const
{
    return d->dom;
}

void io::svg::SvgRenderer::write(QIODevice* device, bool indent)
{
    device->write(d->dom.toByteArray(indent ? 4 : -1));
}
