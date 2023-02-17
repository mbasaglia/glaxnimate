#include "avd_parser.hpp"

#include "io/svg/svg_parser_private.hpp"
#include "model/shapes/trim.hpp"

using namespace glaxnimate::io::svg;
using namespace glaxnimate::io::svg::detail;

#include <QDebug>

class glaxnimate::io::avd::AvdParser::Private : public svg::detail::SvgParserPrivate
{
public:
    Private(
        const QDir& resource_path,
        model::Document* document,
        const std::function<void(const QString&)>& on_warning,
        ImportExport* io,
        QSize forced_size
    ) : SvgParserPrivate(document, on_warning, io, forced_size),
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

                for ( const auto& anim : ElementRange(attr) )
                {
                    if ( anim.tagName() != "objectAnimator" )
                        continue;

                    animations[name][anim.attribute("propertyName")] = anim;
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

    std::pair<QPointF, QVector2D> on_parse_meta(const QDomElement& root) override
    {
        QPointF pos;
        QVector2D scale{1, 1};

        if ( root.hasAttribute("viewportWidth") && root.hasAttribute("viewportHeight") )
        {
            qreal vbw = len_attr(root, "viewportWidth");
            qreal vbh = len_attr(root, "viewportHeight");

            if ( !forced_size.isValid() )
            {
                if ( !root.hasAttribute("width") )
                    size.setWidth(vbw);
                if ( !root.hasAttribute("height") )
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

        return {pos, scale};
    }

    void on_parse_finish(model::Layer* parent_layer, const QDomElement& svg) override
    {
        parent_layer->name.set(
            attr(svg, "android", "name", parent_layer->type_name_human())
        );

        document->main()->name.set(
            attr(svg, "android", "name", "")
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
        else if ( args.element.tagName() == "attr" && args.element.attribute("name").endsWith("drawable") )
        {
            parseshape_drawable(args);
        }
    }

    Style initial_style(const QDomElement&) override
    {
        Style default_style(Style::Map{
            {"fillColor", "black"},
        });
        return default_style;
    }

private:
    struct Resource
    {
        QString name;
        QDomElement element;
        model::Asset* asset = nullptr;
    };

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


        shapes->insert(std::move(stroke));
    }

    void add_fill(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        auto fill = std::make_unique<model::Fill>(document);
        set_styler_style(fill.get(), style.get("fillColor", ""));
        fill->opacity.set(percent_1(style.get("fillAlpha", "1")));

        if ( style.get("fillType", "") == "evenOdd" )
            fill->fill_rule.set(model::Fill::EvenOdd);

        shapes->insert(std::move(fill));
    }

    void add_trim(const ParseFuncArgs& args, model::ShapeListProperty* shapes, const Style& style)
    {
        auto trim = std::make_unique<model::Trim>(document);

        trim->start.set(percent_1(style.get("trimPathStart", "1")));
        trim->end.set(percent_1(style.get("trimPathEnd", "1")));
        trim->offset.set(percent_1(style.get("trimPathOffset", "1")));

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

        // TODO sweep = conical
        QString type = res->element.attribute("type", "linear");
        if ( type == "linear" )
            gradient->type.set(model::Gradient::Linear);
        else if ( type == "radial" )
            gradient->type.set(model::Gradient::Radial);

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

        model::Path* shape = nullptr;
        for ( const auto& bezier : bez.beziers() )
        {
            auto shape = std::make_unique<model::Path>(document);
            shape->shape.set(bezier);
            shape->closed.set(bezier.closed());
            clip->shapes.insert(std::move(shape));
        }

        return clip;
    }

    void parseshape_drawable(const ParseFuncArgs& args)
    {
        std::unique_ptr<model::Layer> layer;
        set_name(layer.get(), args.element);
        parse_children({args.element, &layer->shapes, args.parent_style, false});
        args.shape_parent->insert(std::move(layer));
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
        model::Path* shape = nullptr;
        for ( const auto& bezier : bez.beziers() )
        {
            shape = push<model::Path>(shapes);
            shape->shape.set(bezier);
            shape->closed.set(bezier.closed());
        }
        add_shapes(args, std::move(shapes));
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

    QDir resource_path;
    std::map<QString, Resource> resources;
    int internal_resource_id = 0;
    std::map<QString, std::map<QString, QDomElement>> animations;

    static const std::map<QString, void (Private::*)(const ParseFuncArgs&)> shape_parsers;
    static const std::unordered_set<QString> style_atrrs;
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
    QSize forced_size
)
    : d(std::make_unique<Private>(resource_path, document, on_warning, io, forced_size))
{
    d->load(device);
}

glaxnimate::io::avd::AvdParser::~AvdParser() = default;

void glaxnimate::io::avd::AvdParser::parse_to_document()
{
    d->parse();
}
