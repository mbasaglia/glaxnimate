/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "svg_parser.hpp"
#include "svg_parser_private.hpp"

using namespace glaxnimate::io::svg::detail;

class glaxnimate::io::svg::SvgParser::Private : public SvgParserPrivate
{
public:
    Private(
        model::Document* document,
        const std::function<void(const QString&)>& on_warning,
        ImportExport* io,
        QSize forced_size,
        model::FrameTime default_time,
        GroupMode group_mode,
        QDir default_asset_path
    ) : SvgParserPrivate(document, on_warning, io, forced_size, default_time),
        group_mode(group_mode),
        default_asset_path(default_asset_path)
    {}

protected:
    void on_parse_prepare(const QDomElement&) override
    {
        for ( const auto& p : shape_parsers )
            to_process += dom.elementsByTagName(p.first).count();
    }

    QSizeF get_size(const QDomElement& svg) override
    {
        return {
            len_attr(svg, "width"_qs, size.width()),
            len_attr(svg, "height"_qs, size.height())
        };
    }

    void on_parse(const QDomElement& svg) override
    {
        dpi = attr(svg, "inkscape"_qs, "export-xdpi"_qs, "96"_qs).toDouble();

        QPointF pos;
        QVector2D scale{1, 1};
        if ( svg.hasAttribute("viewBox"_qs) )
        {
            auto vb = split_attr(svg, "viewBox"_qs);
            if ( vb.size() == 4 )
            {
                qreal vbx = vb[0].toDouble();
                qreal vby = vb[1].toDouble();
                qreal vbw = vb[2].toDouble();
                qreal vbh = vb[3].toDouble();

                if ( !forced_size.isValid() )
                {
                    if ( !svg.hasAttribute("width"_qs) )
                        size.setWidth(vbw);
                    if ( !svg.hasAttribute("height"_qs) )
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

        for ( const auto& link_node : ItemCountRange(dom.elementsByTagName("link"_qs)) )
        {
            auto link = link_node.toElement();
            if ( link.attribute("rel"_qs) == "stylesheet"_qs )
            {
                QString url = link.attribute("href"_qs);
                if ( !url.isEmpty() )
                    document->add_pending_asset(""_qs, QUrl(url));
            }
        }

        parse_css();
        parse_assets();
        parse_metadata();

        model::Layer* parent_layer = add_layer(&main->shapes);
        parent_layer->transform.get()->position.set(-pos);
        parent_layer->transform.get()->scale.set(scale);
        parent_layer->name.set(
            attr(svg, "sodipodi"_qs, "docname"_qs, svg.attribute("id"_qs, parent_layer->type_name_human()))
        );

        Style default_style(Style::Map{
            {"fill"_qs, "black"_qs},
        });
        parse_children({svg, &parent_layer->shapes, parse_style(svg, default_style), false});

        main->name.set(
            attr(svg, "sodipodi"_qs, "docname"_qs, ""_qs)
        );
    }

    void parse_shape(const ParseFuncArgs& args) override
    {
        if ( handle_mask(args) )
            return;

        parse_shape_impl(args);
    }

private:
    void parse_css()
    {
        CssParser parser(css_blocks);

        for ( const auto& style : ItemCountRange(dom.elementsByTagName("style"_qs)) )
        {
            QString data;
            for ( const auto & child : ItemCountRange(style.childNodes()) )
            {
                if ( child.isText() || child.isCDATASection() )
                    data += child.toCharacterData().data();
            }

            if ( data.contains("@font-face"_qs) )
                document->add_pending_asset(""_qs, data.toUtf8());

            parser.parse(data);
        }

        std::stable_sort(css_blocks.begin(), css_blocks.end());
    }

    void parse_defs(const QDomNode& node)
    {
        if ( !node.isElement() )
            return;

        auto defs = node.toElement();
        for ( const auto& def : ElementRange(defs) )
        {
            if ( def.tagName().startsWith("animate"_qs) )
            {
                QString link = attr(def, "xlink"_qs, "href"_qs);
                if ( link.isEmpty() || link[0] != '#'_qc )
                    continue;
                animate_parser.store_animate(link.mid(1), def);
            }
        }
    }

    void parse_assets()
    {
        std::vector<QDomElement> later;

        for ( const auto& domnode : ItemCountRange(dom.elementsByTagName("linearGradient"_qs)) )
            parse_gradient_node(domnode, later);

        for ( const auto& domnode : ItemCountRange(dom.elementsByTagName("radialGradient"_qs)) )
            parse_gradient_node(domnode, later);

        std::vector<QDomElement> unprocessed;
        while ( !later.empty() && unprocessed.size() != later.size() )
        {
            unprocessed.clear();

            for ( const auto& element : later )
                parse_brush_style_check(element, unprocessed);

            std::swap(later, unprocessed);
        }


        for ( const auto& defs : ItemCountRange(dom.elementsByTagName("defs"_qs)) )
            parse_defs(defs);
    }

    void parse_gradient_node(const QDomNode& domnode, std::vector<QDomElement>& later)
    {
        if ( !domnode.isElement() )
            return;

        auto gradient = domnode.toElement();
        QString id = gradient.attribute("id"_qs);
        if ( id.isEmpty() )
            return;

        if ( parse_brush_style_check(gradient, later) )
            parse_gradient_nolink(gradient, id);
    }

    bool parse_brush_style_check(const QDomElement& element, std::vector<QDomElement>& later)
    {
        QString link = attr(element, "xlink"_qs, "href"_qs);
        if ( link.isEmpty() )
            return true;

        if ( !link.startsWith("#"_qs) )
            return false;

        auto it = brush_styles.find(link);
        if ( it != brush_styles.end() )
        {
            brush_styles["#"_qs + element.attribute("id"_qs)] = it->second;
            return false;
        }


        auto it1 = gradients.find(link);
        if ( it1 != gradients.end() )
        {
            parse_gradient(element, element.attribute("id"_qs), it1->second);
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

            if ( stop.tagName() != "stop"_qs )
                continue;

            Style style = parse_style(stop, {});
            if ( !style.contains("stop-color"_qs) )
                continue;
            QColor color = parse_color(style["stop-color"_qs], QColor());
            color.setAlphaF(color.alphaF() * style.get("stop-opacity"_qs, "1"_qs).toDouble());

            stops.push_back({stop.attribute("offset"_qs, "0"_qs).toDouble(), color});
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
            brush_styles["#"_qs+id] = col.get();
            auto anim = parse_animated(gradient.firstChildElement("stop"_qs));

            for ( const auto& kf : anim.single("stop-color"_qs) )
                col->color.set_keyframe(kf.time, kf.values.color())->set_transition(kf.transition);

            document->assets()->colors->values.insert(std::move(col));
            return;
        }

        auto colors = std::make_unique<model::GradientColors>(document);
        colors->name.set(id);
        colors->colors.set(stops);
        gradients["#"_qs+id] = colors.get();
        auto ptr = colors.get();
        document->assets()->gradient_colors->values.insert(std::move(colors));
        parse_gradient(gradient, id, ptr);
    }

    void parse_gradient(const QDomElement& element, const QString& id, model::GradientColors* colors)
    {
        auto gradient = std::make_unique<model::Gradient>(document);
        QTransform gradient_transform;

        if ( element.hasAttribute("gradientTransform"_qs) )
            gradient_transform = svg_transform(element.attribute("gradientTransform"_qs), {}).transform;

        if ( element.tagName() == "linearGradient"_qs )
        {
            if ( !element.hasAttribute("x1"_qs) || !element.hasAttribute("x2"_qs) ||
                 !element.hasAttribute("y1"_qs) || !element.hasAttribute("y2"_qs) )
                return;

            gradient->type.set(model::Gradient::Linear);

            gradient->start_point.set(gradient_transform.map(QPointF(
                len_attr(element, "x1"_qs),
                len_attr(element, "y1"_qs)
            )));
            gradient->end_point.set(gradient_transform.map(QPointF(
                len_attr(element, "x2"_qs),
                len_attr(element, "y2"_qs)
            )));

            auto anim = parse_animated(element);
            for ( const auto& kf : anim.joined({"x1"_qs, "y1"_qs}) )
                gradient->start_point.set_keyframe(kf.time, {kf.values[0].vector()[0], kf.values[1].vector()[0]})->set_transition(kf.transition);
            for ( const auto& kf : anim.joined({"x2"_qs, "y2"_qs}) )
                gradient->end_point.set_keyframe(kf.time, {kf.values[0].vector()[0], kf.values[1].vector()[0]})->set_transition(kf.transition);
        }
        else if ( element.tagName() == "radialGradient"_qs )
        {
            if ( !element.hasAttribute("cx"_qs) || !element.hasAttribute("cy"_qs) || !element.hasAttribute("r"_qs) )
                return;

            gradient->type.set(model::Gradient::Radial);

            QPointF c = QPointF(
                len_attr(element, "cx"_qs),
                len_attr(element, "cy"_qs)
            );
            gradient->start_point.set(gradient_transform.map(c));

            if ( element.hasAttribute("fx"_qs) )
                gradient->highlight.set(gradient_transform.map(QPointF(
                    len_attr(element, "fx"_qs),
                    len_attr(element, "fy"_qs)
                )));
            else
                gradient->highlight.set(gradient_transform.map(c));

            gradient->end_point.set(gradient_transform.map(QPointF(
                c.x() + len_attr(element, "r"_qs), c.y()
            )));


            auto anim = parse_animated(element);
            for ( const auto& kf : anim.joined({"cx"_qs, "cy"_qs}) )
                gradient->start_point.set_keyframe(kf.time,
                    gradient_transform.map(QPointF{kf.values[0].vector()[0], kf.values[1].vector()[0]})
                )->set_transition(kf.transition);

            for ( const auto& kf : anim.joined({"fx"_qs, "fy"_qs}) )
                gradient->highlight.set_keyframe(kf.time,
                    gradient_transform.map(QPointF{kf.values[0].vector()[0], kf.values[1].vector()[0]})
                )->set_transition(kf.transition);

            for ( const auto& kf : anim.joined({"cx"_qs, "cy"_qs, "r"_qs}) )
                gradient->end_point.set_keyframe(kf.time,
                    gradient_transform.map(QPointF{kf.values[0].vector()[0] + kf.values[2].vector()[0], kf.values[1].vector()[0]})
                )->set_transition(kf.transition);

        }
        else
        {
            return;
        }

        gradient->name.set(id);
        gradient->colors.set(colors);
        brush_styles["#"_qs+id] = gradient.get();
        document->assets()->gradients->values.insert(std::move(gradient));
    }

    Style parse_style(const QDomElement& element, const Style& parent_style)
    {
        Style style = parent_style;

        auto class_names_list = element.attribute("class"_qs).split(" "_qs,
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

        if ( element.hasAttribute("style"_qs) )
        {
            for ( const auto& item : element.attribute("style"_qs).split(';'_qc) )
            {
                auto split = ::utils::split_ref(item, ':'_qc);
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
            if ( it->second == "inherit"_qs )
            {
                QString parent = parent_style.get(it->first, ""_qs);
                if ( parent.isEmpty() || parent == "inherit"_qs )
                {
                    it = style.map.erase(it);
                    continue;
                }
                it->second = parent;
            }

            ++it;
        }

        if ( !style.contains("fill"_qs) )
            style.set("fill"_qs, parent_style.get("fill"_qs));

        style.color = parse_color(style.get("color"_qs, ""_qs), parent_style.color);
        return style;
    }

    bool handle_mask(const ParseFuncArgs& args)
    {
        QString mask_ref;
        if ( args.element.hasAttribute("clip-path"_qs) )
            mask_ref = args.element.attribute("clip-path"_qs);
        else if ( args.element.hasAttribute("mask"_qs) )
            mask_ref = args.element.attribute("mask"_qs);

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
        auto layer = add_layer(args.shape_parent);
        apply_common_style(layer, args.element, style);
        set_name(layer, args.element);
        layer->mask->mask.set(model::MaskSettings::Alpha);

        QDomElement element = args.element;

        QDomElement trans_copy = dom.createElement("g"_qs);
        trans_copy.setAttribute("style"_qs, element.attribute("style"_qs));
        element.removeAttribute("style"_qs);
        trans_copy.setAttribute("transform"_qs, element.attribute("transform"_qs));
        element.removeAttribute("transform"_qs);

        for ( const auto& attr : detail::css_atrrs )
            element.removeAttribute(attr);

        Style mask_style;
        mask_style["stroke"_qs] = "none"_qs;
        parse_g_to_layer({
            mask_element,
            &layer->shapes,
            mask_style,
            false
        });

        parse_shape_impl({
            element,
            &layer->shapes,
            style,
            false
        });

        parse_transform(trans_copy, layer, layer->transform.get());

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

    void parse_transform(
        const QDomElement& element,
        model::Group* node,
        model::Transform* transform
    )
    {
        auto bb = node->local_bounding_rect(0);
        bool anchor_from_inkscape = false;
        QPointF center = bb.center();
        if ( element.hasAttributeNS(detail::xmlns.at("inkscape"_qs), "transform-center-x"_qs) )
        {
            anchor_from_inkscape = true;
            qreal ix = element.attributeNS(detail::xmlns.at("inkscape"_qs), "transform-center-x"_qs).toDouble();
            qreal iy = -element.attributeNS(detail::xmlns.at("inkscape"_qs), "transform-center-y"_qs).toDouble();
            center += QPointF(ix, iy);
        }

        bool anchor_from_rotate = false;

        if ( element.hasAttribute("transform"_qs) )
        {
            auto trans = svg_transform(
                element.attribute("transform"_qs),
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
        else if ( anchor_from_inkscape )
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

        if ( !anim.apply_motion(transform->position, delta_pos, &node->auto_orient) )
        {
            for ( const auto& kf : anim.single("translate"_qs) )
                transform->position.set_keyframe(kf.time, QPointF{kf.values.vector()[0], kf.values.vector()[1]} + delta_pos)->set_transition(kf.transition);
        }

        for ( const auto& kf : anim.single("scale"_qs) )
            transform->scale.set_keyframe(kf.time, QVector2D(kf.values.vector()[0], kf.values.vector()[1]))->set_transition(kf.transition);

        for ( const auto& kf : anim.single("rotate"_qs) )
        {
            transform->rotation.set_keyframe(kf.time, kf.values.vector()[0])->set_transition(kf.transition);
            if ( kf.values.vector().size() == 3 )
            {
                QPointF p = {kf.values.vector()[1], kf.values.vector()[2]};
                transform->anchor_point.set_keyframe(kf.time, p)->set_transition(kf.transition);
                transform->position.set_keyframe(kf.time, p)->set_transition(kf.transition);
            }
        }
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
                warning("Missing transformation parameters"_qs);
                continue;
            }

            QString name = match.captured(1);

            if ( name == "translate"_qs )
            {
                info.transform.translate(args[0], args.size() > 1 ? args[1] : 0);
            }
            else if ( name == "scale"_qs )
            {
                info.transform.scale(args[0], (args.size() > 1 ? args[1] : args[0]));
            }
            else if ( name == "rotate"_qs )
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
            else if ( name == "skewX"_qs )
            {
                info.transform *= QTransform(
                    1, 0, 0,
                    qTan(args[0]), 1, 0,
                    0, 0, 1
                );
            }
            else if ( name == "skewY"_qs )
            {
                info.transform *= QTransform(
                    1, qTan(args[0]), 0,
                    0, 1, 0,
                    0, 0, 1
                );
            }
            else if ( name == "matrix"_qs )
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
                    warning("Wrong translation matrix"_qs);
                }
            }
            else
            {
                warning(QStringLiteral("Unknown transformation %1").arg(name));
            }

        }
        return info;
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

    void apply_common_style(model::VisualNode* node, const QDomElement& element, const Style& style)
    {
        if ( style.get("display"_qs) == "none"_qs || style.get("visibility"_qs) == "hidden"_qs )
            node->visible.set(false);
        node->locked.set(attr(element, "sodipodi"_qs, "insensitive"_qs) == "true"_qs);
        node->set("opacity"_qs, percent_1(style.get("opacity"_qs, "1"_qs)));
        node->get("transform"_qs).value<model::Transform*>();
    }

    void set_name(model::DocumentNode* node, const QDomElement& element)
    {
        QString name = attr(element, "inkscape"_qs, "label"_qs);
        if ( name.isEmpty() )
        {
            name = attr(element, "android"_qs, "name"_qs);
            if ( name.isEmpty() )
                name = element.attribute("id"_qs);
        }
        node->name.set(name);
    }

    void add_style_shapes(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        QString paint_order = style.get("paint-order"_qs, "normal"_qs);
        if ( paint_order == "normal"_qs )
            paint_order = "fill stroke"_qs;

        for ( const auto& sr : paint_order.split(' '_qc,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::SkipEmptyParts
#else
        QString::SkipEmptyParts
#endif
        ) )
        {
            if ( sr == "fill"_qs )
                add_fill(args, shapes, style);
            else if ( sr == "stroke"_qs )
                add_stroke(args, shapes, style);
        }
    }

    void display_to_opacity(model::VisualNode* node,
                            const detail::AnimateParser::AnimatedProperties& anim,
                            model::AnimatedProperty<float>& opacity,
                            Style* style)
    {
        if ( !anim.has("display"_qs) )
            return;

        if ( opacity.keyframe_count() > 2 )
        {
            warning("Either animate `opacity` or `display`, not both"_qs);
            return;
        }

        if ( style )
            style->map.erase("display"_qs);

        model::KeyframeTransition hold;
        hold.set_hold(true);

        for ( const auto& kf : anim.single("display"_qs) )
        {
            opacity.set_keyframe(kf.time, kf.values.string() == "none"_qs ? 0 : 1)->set_transition(hold);
        }

        node->visible.set(true);
    }

    void add_stroke(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        QString stroke_color = style.get("stroke"_qs, "transparent"_qs);
        if ( stroke_color == "none"_qs )
            return;

        auto stroke = std::make_unique<model::Stroke>(document);
        set_styler_style(stroke.get(), stroke_color, style.color);

        stroke->opacity.set(percent_1(style.get("stroke-opacity"_qs, "1"_qs)));
        stroke->width.set(parse_unit(style.get("stroke-width"_qs, "1"_qs)));

        stroke->cap.set(line_cap(style.get("stroke-linecap"_qs, "butt"_qs)));
        stroke->join.set(line_join(style.get("stroke-linejoin"_qs, "miter"_qs)));
        stroke->miter_limit.set(parse_unit(style.get("stroke-miterlimit"_qs, "4"_qs)));

        auto anim = parse_animated(args.element);
        for ( const auto& kf : anim.single("stroke"_qs) )
            stroke->color.set_keyframe(kf.time, kf.values.color())->set_transition(kf.transition);

        for ( const auto& kf : anim.single("stroke-opacity"_qs) )
            stroke->opacity.set_keyframe(kf.time, kf.values.vector()[0])->set_transition(kf.transition);

        for ( const auto& kf : anim.single("stroke-width"_qs) )
            stroke->width.set_keyframe(kf.time, kf.values.vector()[0])->set_transition(kf.transition);

        display_to_opacity(stroke.get(), anim, stroke->opacity, nullptr);

        shapes->insert(std::move(stroke));
    }

    void set_styler_style(model::Styler* styler, const QString& color_str, const QColor& current_color)
    {
        if ( !color_str.startsWith("url"_qs) )
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
        QString fill_color = style.get("fill"_qs, ""_qs);

        auto fill = std::make_unique<model::Fill>(document);
        set_styler_style(fill.get(), fill_color, style.color);
        fill->opacity.set(percent_1(style.get("fill-opacity"_qs, "1"_qs)));

        if ( style.get("fill-rule"_qs, ""_qs) == "evenodd"_qs )
            fill->fill_rule.set(model::Fill::EvenOdd);

        auto anim = parse_animated(args.element);
        for ( const auto& kf : anim.single("fill"_qs) )
            fill->color.set_keyframe(kf.time, kf.values.color())->set_transition(kf.transition);

        for ( const auto& kf : anim.single("fill-opacity"_qs) )
            fill->opacity.set_keyframe(kf.time, kf.values.vector()[0])->set_transition(kf.transition);

        if ( fill_color == "none"_qs )
            fill->visible.set(false);

        display_to_opacity(fill.get(), anim, fill->opacity, nullptr);

        shapes->insert(std::move(fill));
    }

    QColor parse_color(const QString& color_str, const QColor& current_color)
    {
        if ( color_str.isEmpty() || color_str == "currentColor"_qs )
            return current_color;

        return glaxnimate::io::svg::parse_color(color_str);
    }

    void parseshape_rect(const ParseFuncArgs& args)
    {
        ShapeCollection shapes;
        auto rect = push<model::Rect>(shapes);
        qreal w = len_attr(args.element, "width"_qs, 0);
        qreal h = len_attr(args.element, "height"_qs, 0);
        rect->position.set(QPointF(
            len_attr(args.element, "x"_qs, 0) + w / 2,
            len_attr(args.element, "y"_qs, 0) + h / 2
        ));
        rect->size.set(QSizeF(w, h));
        qreal rx = len_attr(args.element, "rx"_qs, 0);
        qreal ry = len_attr(args.element, "ry"_qs, 0);
        rect->rounded.set(qMax(rx, ry));


        auto anim = parse_animated(args.element);

        /// \todo handle offset
        anim.apply_motion(rect->position);

        for ( const auto& kf : anim.joined({"x"_qs, "y"_qs, "width"_qs, "height"_qs}) )
            rect->position.set_keyframe(kf.time, {
                kf.values[0].vector()[0] + kf.values[2].vector()[0] / 2,
                kf.values[1].vector()[0] + kf.values[3].vector()[0] / 2
            })->set_transition(kf.transition);

        for ( const auto& kf : anim.joined({"width"_qs, "height"_qs}) )
            rect->size.set_keyframe(kf.time, {kf.values[0].vector()[0], kf.values[1].vector()[0]})->set_transition(kf.transition);

        for ( const auto& kf : anim.joined({"rx"_qs, "ry"_qs}) )
            rect->rounded.set_keyframe(kf.time, qMax(kf.values[0].vector()[0], kf.values[1].vector()[0]))->set_transition(kf.transition);

        add_shapes(args, std::move(shapes));
    }

    void parseshape_ellipse(const ParseFuncArgs& args)
    {
        ShapeCollection shapes;
        auto ellipse = push<model::Ellipse>(shapes);
        ellipse->position.set(QPointF(
            len_attr(args.element, "cx"_qs, 0),
            len_attr(args.element, "cy"_qs, 0)
        ));
        qreal rx = len_attr(args.element, "rx"_qs, 0);
        qreal ry = len_attr(args.element, "ry"_qs, 0);
        ellipse->size.set(QSizeF(rx * 2, ry * 2));

        auto anim = parse_animated(args.element);
        anim.apply_motion(ellipse->position);
        for ( const auto& kf : anim.joined({"cx"_qs, "cy"_qs}) )
            ellipse->position.set_keyframe(kf.time, {kf.values[0].vector()[0], kf.values[1].vector()[0]})->set_transition(kf.transition);
        for ( const auto& kf : anim.joined({"rx"_qs, "ry"_qs}) )
            ellipse->size.set_keyframe(kf.time, {kf.values[0].vector()[0]*2, kf.values[1].vector()[0]*2})->set_transition(kf.transition);

        add_shapes(args, std::move(shapes));
    }

    void parseshape_circle(const ParseFuncArgs& args)
    {
        ShapeCollection shapes;
        auto ellipse = push<model::Ellipse>(shapes);
        ellipse->position.set(QPointF(
            len_attr(args.element, "cx"_qs, 0),
            len_attr(args.element, "cy"_qs, 0)
        ));
        qreal d = len_attr(args.element, "r"_qs, 0) * 2;
        ellipse->size.set(QSizeF(d, d));

        auto anim = parse_animated(args.element);
        anim.apply_motion(ellipse->position);
        for ( const auto& kf : anim.joined({"cx"_qs, "cy"_qs}) )
            ellipse->position.set_keyframe(kf.time, {kf.values[0].vector()[0], kf.values[1].vector()[0]})->set_transition(kf.transition);
        for ( const auto& kf : anim.single({"r"_qs}) )
            ellipse->size.set_keyframe(kf.time, {kf.values.vector()[0]*2, kf.values.vector()[0]*2})->set_transition(kf.transition);

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
                else if ( attr(args.element, "inkscape"_qs, "groupmode"_qs) == "layer"_qs )
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
            layer->transform.get(),
            style
        );
    }

    void parse_g_to_shape(const ParseFuncArgs& args)
    {
        Style style = parse_style(args.element, args.parent_style);
        auto ugroup = std::make_unique<model::Group>(document);
        auto group = ugroup.get();
        args.shape_parent->insert(std::move(ugroup));
        parse_g_common(
            {args.element, &group->shapes, style, true},
            group,
            group->transform.get(),
            style
        );
    }

    void parse_g_common(
        const ParseFuncArgs& args,
        model::Group* g_node,
        model::Transform* transform,
        Style& style
    )
    {
        apply_common_style(g_node, args.element, args.parent_style);

        auto anim = parse_animated(args.element);

        for ( const auto& kf : anim.single("opacity"_qs) )
            g_node->opacity.set_keyframe(kf.time, kf.values.vector()[0])->set_transition(kf.transition);

        display_to_opacity(g_node, anim, g_node->opacity, &style);

        set_name(g_node, args.element);
        // Avoid doubling opacity values
        style.map.erase("opacity"_qs);
        parse_children(args);
        parse_transform(args.element, g_node, transform);
    }

    std::vector<model::Path*> parse_bezier_impl(const ParseFuncArgs& args, const math::bezier::MultiBezier& bez)
    {
        if ( bez.beziers().empty() )
            return {};

        ShapeCollection shapes;
        std::vector<model::Path*> paths;
        for ( const auto& bezier : bez.beziers() )
        {
            model::Path* shape = push<model::Path>(shapes);
            paths.push_back(shape);
            shape->shape.set(bezier);
            shape->closed.set(bezier.closed());
        }
        add_shapes(args, std::move(shapes));
        return paths;
    }


    model::Path* parse_bezier_impl_single(const ParseFuncArgs& args, const math::bezier::Bezier& bez)
    {
        ShapeCollection shapes;
        auto path = push<model::Path>(shapes);
        path->shape.set(bez);
        add_shapes(args, {std::move(shapes)});
        return path;
    }

    detail::AnimateParser::AnimatedProperties parse_animated(const QDomElement& element)
    {
        return animate_parser.parse_animated_properties(element);
    }

    void parseshape_line(const ParseFuncArgs& args)
    {
        math::bezier::Bezier bez;
        bez.add_point(QPointF(
            len_attr(args.element, "x1"_qs, 0),
            len_attr(args.element, "y1"_qs, 0)
        ));
        bez.line_to(QPointF(
            len_attr(args.element, "x2"_qs, 0),
            len_attr(args.element, "y2"_qs, 0)
        ));
        auto path = parse_bezier_impl_single(args, bez);
        for ( const auto& kf : parse_animated(args.element).joined({"x1"_qs, "y1"_qs, "x2"_qs, "y2"_qs}) )
        {
            math::bezier::Bezier bez;
            bez.add_point({kf.values[0].vector()[0], kf.values[1].vector()[0]});
            bez.add_point({kf.values[2].vector()[0], kf.values[3].vector()[0]});
            path->shape.set_keyframe(kf.time, bez)->set_transition(kf.transition);
        }
    }

    math::bezier::Bezier build_poly(const std::vector<qreal>& coords, bool close)
    {
        math::bezier::Bezier bez;

        if ( coords.size() < 4 )
        {
            if ( !coords.empty() )
                warning("Not enough `points` for `polygon` / `polyline`"_qs);
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
        auto path = parse_bezier_impl_single(args, build_poly(double_args(args.element.attribute("points"_qs, ""_qs)), close));
        if ( !path )
            return;

        for ( const auto& kf : parse_animated(args.element).single("points"_qs) )
            path->shape.set_keyframe(kf.time, build_poly(kf.values.vector(), close))->set_transition(kf.transition);

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
        QString d = args.element.attribute("d"_qs);
        math::bezier::MultiBezier bez = PathDParser(d).parse();
        /// \todo sodipodi:nodetypes
        auto paths = parse_bezier_impl(args, bez);

        path_animation(paths, parse_animated(args.element), "d"_qs );
    }

    bool parse_star(const ParseFuncArgs& args)
    {
        if ( attr(args.element, "sodipodi"_qs, "type"_qs) != "star"_qs )
            return false;

        qreal randomized = attr(args.element, "inkscape"_qs, "randomized"_qs, "0"_qs).toDouble();
        if ( !qFuzzyCompare(randomized, 0.0) )
            return false;

        qreal rounded = attr(args.element, "inkscape"_qs, "rounded"_qs, "0"_qs).toDouble();
        if ( !qFuzzyCompare(rounded, 0.0) )
            return false;


        ShapeCollection shapes;
        auto shape = push<model::PolyStar>(shapes);
        shape->points.set(
            attr(args.element, "sodipodi"_qs, "sides"_qs).toInt()
        );
        auto flat = attr(args.element, "inkscape"_qs, "flatsided"_qs);
        shape->type.set(
            flat == "true"_qs ?
            model::PolyStar::Polygon :
            model::PolyStar::Star
        );
        shape->position.set(QPointF(
            attr(args.element, "sodipodi"_qs, "cx"_qs).toDouble(),
            attr(args.element, "sodipodi"_qs, "cy"_qs).toDouble()
        ));
        shape->outer_radius.set(attr(args.element, "sodipodi"_qs, "r1"_qs).toDouble());
        shape->inner_radius.set(attr(args.element, "sodipodi"_qs, "r2"_qs).toDouble());
        shape->angle.set(
            math::rad2deg(attr(args.element, "sodipodi"_qs, "arg1"_qs).toDouble())
            +90
        );

        add_shapes(args, std::move(shapes));
        return true;
    }

    void parseshape_use(const ParseFuncArgs& args)
    {
        QString id = attr(args.element, "xlink"_qs, "href"_qs);
        if ( !id.startsWith('#'_qc) )
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
            QPointF(len_attr(args.element, "x"_qs, 0), len_attr(args.element, "y"_qs, 0))
        );
        parse_transform(args.element, group.get(), group->transform.get());
        args.shape_parent->insert(std::move(group));
    }

    QString find_asset_file(const QString& path)
    {
        QFileInfo finfo(path);
        if ( finfo.exists() )
            return path;
        else if ( default_asset_path.exists(path) )
            return default_asset_path.filePath(path);
        else if ( default_asset_path.exists(finfo.fileName()) )
            return default_asset_path.filePath(finfo.fileName());

        return {};
    }

    bool open_asset_file(model::Bitmap* image, const QString& path)
    {
        if ( path.isEmpty() )
            return false;

        auto file = find_asset_file(path);
        if ( file.isEmpty() )
            return false;

        return image->from_file(file);
    }

    void parseshape_image(const ParseFuncArgs& args)
    {
        auto bitmap = std::make_unique<model::Bitmap>(document);

        bool open = false;
        QString href = attr(args.element, "xlink"_qs, "href"_qs);
        QUrl url = QUrl(href);

        if ( url.isRelative() )
            open = open_asset_file(bitmap.get(), href);
        if ( !open )
        {
            if ( url.isLocalFile() )
                open = open_asset_file(bitmap.get(), url.toLocalFile());
            else
                open = bitmap->from_url(url);
        }

        if ( !open )
        {
            QString path = attr(args.element, "sodipodi"_qs, "absref"_qs);
            open = open_asset_file(bitmap.get(), path);
        }
        if ( !open )
            warning(QStringLiteral("Could not load image %1").arg(href));

        auto image = std::make_unique<model::Image>(document);
        image->image.set(document->assets()->images->values.insert(std::move(bitmap)));

        QTransform trans;
        if ( args.element.hasAttribute("transform"_qs) )
            trans = svg_transform(args.element.attribute("transform"_qs), trans).transform;
        trans.translate(
            len_attr(args.element, "x"_qs, 0),
            len_attr(args.element, "y"_qs, 0)
        );
        image->transform->set_transform_matrix(trans);

        args.shape_parent->insert(std::move(image));
    }

    struct TextStyle
    {
        QString family = "sans-serif"_qs;
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

        if ( style.contains("font-family"_qs) )
            out.family = style["font-family"_qs];

        if ( style.contains("font-style"_qs) )
        {
            QString slant = style["font-style"_qs];
            if ( slant == "normal"_qs ) out.style = QFont::StyleNormal;
            else if ( slant == "italic"_qs ) out.style = QFont::StyleItalic;
            else if ( slant == "oblique"_qs ) out.style = QFont::StyleOblique;
        }

        if ( style.contains("font-size"_qs) )
        {
            QString size = style["font-size"_qs];
            static const std::map<QString, int> size_names = {
                {{"xx-small"_qs}, {8}},
                {{"x-small"_qs}, {16}},
                {{"small"_qs}, {32}},
                {{"medium"_qs}, {64}},
                {{"large"_qs}, {128}},
                {{"x-large"_qs}, {256}},
                {{"xx-large"_qs}, {512}},
            };
            if ( size == "smaller"_qs )
                out.size /= 2;
            else if ( size == "larger"_qs )
                out.size *= 2;
            else if ( size_names.count(size) )
                out.size = size_names.at(size);
            else
                out.size = parse_unit(size);
        }

        if ( style.contains("font-weight"_qs) )
        {
            QString weight = style["font-weight"_qs];
            if ( weight == "bold"_qs )
                out.weight = 700;
            else if ( weight == "normal"_qs )
                out.weight = 400;
            else if ( weight == "bolder"_qs )
                out.weight = qMin(1000, out.weight + 100);
            else if ( weight == "lighter"_qs)
                out.weight = qMax(1, out.weight - 100);
            else
                out.weight = weight.toInt();
        }

        if ( style.contains("line-height"_qs) )
            out.line_spacing = parse_unit(style["line-height"_qs]);


        if ( args.element.hasAttribute("xml:space"_qs) )
            out.keep_space = args.element.attribute("xml:space"_qs) == "preserve"_qs;

        if ( args.element.hasAttribute("x"_qs) )
            out.pos.setX(len_attr(args.element, "x"_qs, 0));
        if ( args.element.hasAttribute("y"_qs) )
            out.pos.setY(len_attr(args.element, "y"_qs, 0));

        return out;
    }

    QString trim_text(const QString& text)
    {
        QString trimmed = text.simplified();
        if ( !text.isEmpty() && text.back().isSpace() )
            trimmed += ' '_qc;
        return trimmed;
    }

    void apply_text_style(model::Font* font, const TextStyle& style)
    {
        font->family.set(style.family);
        font->size.set(unit_convert(style.size, "px"_qs, "pt"_qs));
        QFont qfont;
        qfont.setFamily(style.family);
        qfont.setWeight(QFont::Weight(WeightConverter::convert(style.weight, WeightConverter::css, WeightConverter::qt)));
        qfont.setStyle(style.style);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QString style_string = QFontDatabase::styleString(qfont);
#else
        QFontDatabase db;
        QString style_string = db.styleString(qfont);
#endif
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

                    for ( const auto& kf : anim.joined({"x"_qs, "y"_qs}) )
                    {
                        last->position.set_keyframe(
                            kf.time,
                            offset + QPointF(kf.values[0].vector()[0], kf.values[1].vector()[0])
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

    void parse_metadata()
    {
        auto meta = dom.elementsByTagNameNS(xmlns.at("cc"_qs), "Work"_qs);
        if ( meta.count() == 0 )
            return;

        auto work = query_element({"metadata"_qs, "RDF"_qs, "Work"_qs}, dom.documentElement());
        document->info().author = query({"creator"_qs, "Agent"_qs, "title"_qs}, work);
        document->info().description = query({"description"_qs}, work);
        for ( const auto& domnode : ItemCountRange(query_element({"subject"_qs, "Bag"_qs}, work).childNodes()) )
        {
            if ( domnode.isElement() )
            {
                auto child = domnode.toElement();
                if ( child.tagName() == "li"_qs )
                    document->info().keywords.push_back(child.text());

            }
        }
    }

    GroupMode group_mode;
    std::vector<CssStyleBlock> css_blocks;
    QDir default_asset_path;

    static const std::map<QString, void (Private::*)(const ParseFuncArgs&)> shape_parsers;
    static const QRegularExpression transform_re;
    static const QRegularExpression url_re;
};

const std::map<QString, void (glaxnimate::io::svg::SvgParser::Private::*)(const glaxnimate::io::svg::SvgParser::Private::ParseFuncArgs&)> glaxnimate::io::svg::SvgParser::Private::shape_parsers = {
    {"g"_qs,       &glaxnimate::io::svg::SvgParser::Private::parseshape_g},
    {"rect"_qs,    &glaxnimate::io::svg::SvgParser::Private::parseshape_rect},
    {"ellipse"_qs, &glaxnimate::io::svg::SvgParser::Private::parseshape_ellipse},
    {"circle"_qs,  &glaxnimate::io::svg::SvgParser::Private::parseshape_circle},
    {"line"_qs,    &glaxnimate::io::svg::SvgParser::Private::parseshape_line},
    {"polyline"_qs,&glaxnimate::io::svg::SvgParser::Private::parseshape_polyline},
    {"polygon"_qs, &glaxnimate::io::svg::SvgParser::Private::parseshape_polygon},
    {"path"_qs,    &glaxnimate::io::svg::SvgParser::Private::parseshape_path},
    {"use"_qs,     &glaxnimate::io::svg::SvgParser::Private::parseshape_use},
    {"image"_qs,   &glaxnimate::io::svg::SvgParser::Private::parseshape_image},
    {"text"_qs,    &glaxnimate::io::svg::SvgParser::Private::parseshape_text},
};
const QRegularExpression glaxnimate::io::svg::detail::SvgParserPrivate::unit_re{R"(([-+]?(?:[0-9]*\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?)([a-z]*))"_qs};
const QRegularExpression glaxnimate::io::svg::SvgParser::Private::transform_re{R"(([a-zA-Z]+)\s*\(([^\)]*)\))"_qs};
const QRegularExpression glaxnimate::io::svg::SvgParser::Private::url_re{R"(url\s*\(\s*(#[-a-zA-Z0-9_]+)\s*\)\s*)"_qs};
const QRegularExpression glaxnimate::io::svg::detail::AnimateParser::separator{"\\s*,\\s*|\\s+"_qs};
const QRegularExpression glaxnimate::io::svg::detail::AnimateParser::clock_re{R"((?:(?:(?<hours>[0-9]+):)?(?:(?<minutes>[0-9]{2}):)?(?<seconds>[0-9]+(?:\.[0-9]+)?))|(?:(?<timecount>[0-9]+(?:\.[0-9]+)?)(?<unit>h|min|s|ms)))"_qs};
const QRegularExpression glaxnimate::io::svg::detail::AnimateParser::frame_separator_re{"\\s*;\\s*"_qs};

glaxnimate::io::svg::SvgParser::SvgParser(
    QIODevice* device,
    GroupMode group_mode,
    model::Document* document,
    const std::function<void(const QString&)>& on_warning,
    ImportExport* io,
    QSize forced_size,
    model::FrameTime default_time,
    QDir default_asset_path
)
    : d(std::make_unique<Private>(document, on_warning, io, forced_size, default_time, group_mode, default_asset_path))
{
    d->load(device);
}

glaxnimate::io::svg::SvgParser::~SvgParser()
{
}


glaxnimate::io::mime::DeserializedData glaxnimate::io::svg::SvgParser::parse_to_objects()
{
    glaxnimate::io::mime::DeserializedData data;
    data.initialize_data();
    d->parse(data.document.get());
    return data;
}

void glaxnimate::io::svg::SvgParser::parse_to_document()
{
    d->parse();
}

static qreal hex(const QString& s, int start, int size)
{
    return utils::mid_ref(s, start, size).toInt(nullptr, 16) / (size == 2 ? 255.0 : 15.0);
}

QColor glaxnimate::io::svg::parse_color(const QString& string)
{
    if ( string.isEmpty() )
        return {};

    // #fff #112233
    if ( string[0] == '#'_qc )
    {
        if ( string.size() == 4 || string.size() == 5 )
        {
            qreal alpha = string.size() == 4 ? 1. : hex(string, 4, 1);
            return QColor::fromRgbF(hex(string, 1, 1), hex(string, 2, 1), hex(string, 3, 1), alpha);
        }
        else if ( string.size() == 7 || string.size() == 9 )
        {
            qreal alpha = string.size() == 7 ? 1. : hex(string, 7, 2);
            return QColor::fromRgbF(hex(string, 1, 2), hex(string, 3, 2), hex(string, 5, 2), alpha);
        }
        return QColor();
    }

    // transparent
    if ( string == "transparent"_qs || string == "none"_qs )
        return QColor(0, 0, 0, 0);

    QRegularExpressionMatch match;

    // rgba(123, 123, 123, 0.7)
    static QRegularExpression rgba{R"(^rgba\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9.eE]+)\s*\)$)"_qs};
    match = rgba.match(string);
    if ( match.hasMatch() )
        return QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt(), match.captured(4).toDouble() * 255);

    // rgb(123, 123, 123)
    static QRegularExpression rgb{R"(^rgb\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\)$)"_qs};
    match = rgb.match(string);
    if ( match.hasMatch() )
        return QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());

    // rgba(60%, 30%, 20%, 0.7)
    static QRegularExpression rgba_pc{R"(^rgba\s*\(\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)\s*\)$)"_qs};
    match = rgba_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromRgbF(match.captured(1).toDouble() / 100, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100, match.captured(4).toDouble());

    // rgb(60%, 30%, 20%)
    static QRegularExpression rgb_pc{R"(^rgb\s*\(\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*\)$)"_qs};
    match = rgb_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromRgbF(match.captured(1).toDouble() / 100, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100);

    // hsl(60, 30%, 20%)
    static QRegularExpression hsl{R"(^hsl\s*\(\s*([0-9.eE]+)\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*\)$)"_qs};
    match = rgb_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromHslF(match.captured(1).toDouble() / 360, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100);

    // hsla(60, 30%, 20%, 0.7)
    static QRegularExpression hsla{R"(^hsla\s*\(\s*([0-9.eE]+)\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)%\s*,\s*([0-9.eE]+)\s*\)$)"_qs};
    match = rgb_pc.match(string);
    if ( match.hasMatch() )
        return QColor::fromHslF(match.captured(1).toDouble() / 360, match.captured(2).toDouble() / 100, match.captured(3).toDouble() / 100, match.captured(4).toDouble());

    // red
    return QColor(string);
}
