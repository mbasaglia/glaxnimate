#include "avd_renderer.hpp"

#include "io/svg/detail.hpp"
#include "io/svg/svg_renderer.hpp"

#include "model/document.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/trim.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/path.hpp"

class glaxnimate::io::avd::AvdRenderer::Private
{
public:
    void render(model::Document* doc)
    {
        vector = dom.createElement("vector");
        vector.setAttribute("android:width", QString("%1dp").arg(doc->main()->width.get()));
        vector.setAttribute("android:height", QString("%1dp").arg(doc->main()->height.get()));
        vector.setAttribute("android:viewportWidth", QString::number(doc->main()->width.get()));
        vector.setAttribute("android:viewportHeight", QString::number(doc->main()->height.get()));

        render_comp(doc->main(), vector);
    }

    void render_comp(model::Composition* comp, QDomElement& parent)
    {
        for ( const auto& layer : comp->shapes )
            render_element(layer.get(), parent);
    }

    void render_element(model::ShapeElement* elm, QDomElement& parent)
    {
        if ( auto l = elm->cast<model::Layer>() )
        {
            render_layer(l, parent);
        }
        else if ( auto g = elm->cast<model::Group>() )
        {
            render_group(g, parent);
        }
        else if ( elm->is_instance<model::Shape>() )
        {
            warning(QObject::tr("%s should be in a group").arg(elm->object_name()));
        }
        else if ( !elm->is_instance<model::Styler>() && !elm->is_instance<model::Trim>() )
        {
            warning(QObject::tr("%s is not supported").arg(elm->type_name_human()));
        }
    }

    void warning(const QString& s)
    {
        if ( on_warning )
            on_warning(s);
    }

    QDomElement render_layer_parents(model::Layer* lay, QDomElement& parent)
    {
        if ( auto parlay = lay->parent.get() )
        {
            auto p = render_layer_parents(parlay, parent);
            QDomElement group = dom.createElement("group");
            p.appendChild(group);
            render_transform(parlay->transform.get(), group, unique_name(parlay, true) );
            return p;
        }

        return parent;
    }

    void render_layer(model::Layer* lay, QDomElement& parent)
    {
        auto parent_element = parent;
        QDomElement p = render_layer_parents(lay, parent);
        auto elm = render_group(lay, p);
        if ( lay->mask->mask.get() != model::MaskSettings::NoMask )
        {
            auto mask = render_clip_path(lay->shapes[0]);
            elm.insertBefore(mask, {});
        }
    }

    QString unique_name(model::DocumentNode* node, bool is_duplicate)
    {
        QString base = node->name.get();
        if ( base.isEmpty() )
            base = "item_" + node->uuid.get().toString(QUuid::Id128);


        QString name = base;
        if ( is_duplicate )
            name += "_" + QString::number(unique_id++);

        while ( names.count(name) )
        {
            name = base + "_" + QString::number(unique_id++);
        }

        names.insert(name);
        return name;
    }

    QDomElement render_group(model::Group* group, QDomElement& parent)
    {
        QDomElement elm = dom.createElement("group");
        parent.appendChild(elm);
        render_transform(group->transform.get(), elm, unique_name(group, false));

        model::Fill* fill = nullptr;
        model::Stroke* stroke = nullptr;
        model::Trim* trim = nullptr;
        std::vector<std::variant<model::Shape*, model::Group*>> children;
        std::vector<model::Shape*> shapes;
        std::vector<model::Group*> groups;

        for ( const auto& ch : group->shapes )
        {
            if ( auto f = ch->cast<model::Fill>() )
                fill = f;
            else if ( auto s = ch->cast<model::Stroke>() )
                stroke = s;
            else if ( auto t = ch->cast<model::Trim>() )
                trim = t;
            else if ( auto g = ch->cast<model::Group>() )
            {
                groups.push_back(g);
                children.push_back(g);
            }
            else if ( auto s = ch->cast<model::Shape>() )
            {
                shapes.push_back(s);
                children.push_back(s);
            }
            else
            {
                warning(QObject::tr("%s are not supported").arg(ch->type_name_human()));
            }
        }

        bool unify_shapes = !groups.empty();
        if ( !shapes.empty() )
            unify_shapes = true;
        else if ( trim )
            unify_shapes = trim->multiple.get() == model::Trim::Simultaneously;


        if ( unify_shapes )
        {
            for ( const auto& g : groups )
                render_group(g, elm);

            if ( !shapes.empty() )
            {
                QString name = shapes.size() == 1 ? unique_name(shapes[0], false) : unique_name(group, true);
                render_shapes(shapes, name, elm, fill, stroke, trim);
            }
        }
        else
        {
            for ( const auto& ch : children )
            {
                if ( ch.index() == 0 )
                    render_shapes({std::get<0>(ch)}, unique_name(std::get<0>(ch), false), elm, fill, stroke, trim);
                else
                    render_group(std::get<1>(ch), elm);
            }
        }

        return elm;
    }


    void render_shapes(const std::vector<model::Shape*>& shapes, const QString& name, QDomElement& parent,
                       model::Fill* fill, model::Stroke* stroke, model::Trim* trim)
    {
        if ( shapes.empty() )
            return;

        auto path = dom.createElement("path");
        parent.appendChild(path);
        path.setAttribute("android:name", name);
        render_shapes_to_path_data(shapes, name, path);
        render_fill(fill, name, path);
        render_stroke(stroke, name, path);
        render_trim(trim, name, path);
    }

    void render_shapes_to_path_data(const std::vector<model::Shape*>& shapes, const QString& name, QDomElement& elem)
    {
        std::vector<std::unique_ptr<model::ShapeElement>> saved;
        std::vector<model::Path*> paths;
        paths.reserve(shapes.size());

        for ( const auto& sh : shapes )
        {
            if ( auto p = sh->cast<model::Path>() )
            {
                paths.push_back(p);
            }
            else
            {
                auto conv = sh->to_path();
                collect_paths(conv.get(), paths);
                saved.push_back(std::move(conv));
            }
        }

        elem.setAttribute("android:pathData", paths_to_path_data(paths, shapes[0]->time()));

    }

    void collect_paths(model::ShapeElement* element, std::vector<model::Path*>& paths)
    {
        if ( auto p = element->cast<model::Path>() )
        {
            paths.push_back(p);
        }
        else if ( auto g = element->cast<model::Group>() )
        {
            for ( const auto& c : g->shapes )
                collect_paths(c.get(), paths);
        }
    }

    QString paths_to_path_data(const std::vector<model::Path*>& paths, model::FrameTime t)
    {
        math::bezier::MultiBezier bez;
        for ( const auto& path : paths )
            bez.beziers().push_back(path->to_bezier(t));

        return svg::path_data(bez).first;
    }

    void render_fill(model::Fill* fill, const QString& name, QDomElement& element)
    {
        if ( !fill )
            return;

        render_styler_color(fill, name, "android:fillColor", element);
        element.setAttribute("android:fillAlpha", QString::number(fill->opacity.get()));
        element.setAttribute("android:fillType", fill->fill_rule.get() == model::Fill::EvenOdd ? "evenOdd" : "nonZero");

    }

    void render_stroke(model::Stroke* stroke, const QString& name, QDomElement& element)
    {
        if ( !stroke )
            return;

        render_styler_color(stroke, name, "android:strokeColor", element);

        element.setAttribute("android:strokeAlpha", QString::number(stroke->opacity.get()));
        element.setAttribute("android:strokeWidth", QString::number(stroke->width.get()));
        element.setAttribute("android:strokeMiterLimit", QString::number(stroke->miter_limit.get()));
        switch ( stroke->cap.get() )
        {
            case model::Stroke::RoundCap:
                element.setAttribute("android:strokeLineCap", "round");
                break;
            case model::Stroke::ButtCap:
                element.setAttribute("android:strokeLineCap", "butt");
                break;
            case model::Stroke::SquareCap:
                element.setAttribute("android:strokeLineCap", "square");
                break;
        }
        switch ( stroke->join.get() )
        {
            case model::Stroke::RoundJoin:
                element.setAttribute("android:strokeLineJoin", "round");
                break;
            case model::Stroke::MiterJoin:
                element.setAttribute("android:strokeLineJoin", "miter");
                break;
            case model::Stroke::BevelJoin:
                element.setAttribute("android:strokeLineJoin", "bevel");
                break;
        }

    }

    void render_styler_color(model::Styler* styler, const QString& name, const QString& attr, QDomElement& element)
    {
        auto use = styler->use.get();

        if ( auto color = use->cast<model::NamedColor>() )
        {
            element.setAttribute(attr, render_color(color->color.get()));
        }
        // TODO gradient
        else
        {
            element.setAttribute(attr, render_color(styler->color.get()));
        }
    }

    void render_trim(model::Trim* trim, const QString& name, QDomElement& element)
    {
        if ( !trim )
            return;

        element.setAttribute("android:trimPathStart", QString::number(trim->start.get()));
        element.setAttribute("android:trimPathEnd", QString::number(trim->end.get()));
        element.setAttribute("android:trimPathOffset", QString::number(trim->offset.get()));
    }

    QDomElement render_clip_path(model::ShapeElement* element)
    {
        QDomElement clip = dom.createElement("clip-path");
        QString name = unique_name(element, false);
        clip.setAttribute("android:name", name);

        if ( auto group = element->cast<model::Group>() )
        {
            std::vector<model::Shape*> shapes = group->docnode_find_by_type<model::Shape>();
            render_shapes_to_path_data(shapes, name, clip);
        }
        else if ( auto shape = element->cast<model::Shape>() )
        {
            render_shapes_to_path_data({shape}, name, clip);
        }
        else
        {
            warning(QObject::tr("%s cannot be a clip path").arg(element->object_name()));
            return {};
        }

        return clip;
    }

    QString color_comp(int comp)
    {
        return QString::number(comp, 16).rightJustified(2, '0');
    }

    QString render_color(const QColor& color)
    {
        return "#" + color_comp(color.alpha()) + color_comp(color.red()) + color_comp(color.green()) + color_comp(color.blue());
    }

    void render_transform(model::Transform* trans, QDomElement& elm, const QString& name)
    {
        auto ap = trans->anchor_point.get();
        elm.setAttribute("android:pivotX", QString::number(ap.x()));
        elm.setAttribute("android:pivotY", QString::number(ap.y()));
        auto pos = trans->position.get() - ap;
        elm.setAttribute("android:translateX", QString::number(pos.x()));
        elm.setAttribute("android:translateY", QString::number(pos.y()));
        auto scale = trans->scale.get();
        elm.setAttribute("android:scaleX", QString::number(scale.x()));
        elm.setAttribute("android:scaleY", QString::number(scale.y()));
        elm.setAttribute("android:rotation", QString::number(trans->rotation.get()));
    }

    int unique_id = 0;
    QDomDocument dom;
    QDomElement vector;
    std::vector<QDomElement> targets;
    std::function<void (const QString &)> on_warning;
    std::unordered_set<QString> names;
};

glaxnimate::io::avd::AvdRenderer::AvdRenderer(const std::function<void (const QString &)>& on_warning)
    : d(std::make_unique<Private>())
{
    d->on_warning = on_warning;
}

glaxnimate::io::avd::AvdRenderer::~AvdRenderer()
{
}

void glaxnimate::io::avd::AvdRenderer::render(model::Document* document)
{
    d->render(document);
}

QDomElement glaxnimate::io::avd::AvdRenderer::graphics()
{
    return d->vector;
}

const std::vector<QDomElement> & glaxnimate::io::avd::AvdRenderer::animations()
{
    return d->targets;
}

QDomDocument glaxnimate::io::avd::AvdRenderer::single_file()
{
    QDomDocument dom;
    auto av = dom.createElement("animated-vector");
    dom.appendChild(av);
    av.setAttribute("xmlns", svg::detail::xmlns.at("android"));
    for ( const auto& p : svg::detail::xmlns )
    {
        if ( !p.second.contains("android") )
            av.setAttribute("xmlns:" + p.first, p.second);
    }

    auto attr = dom.createElement("aapt:attr");
    av.appendChild(attr);
    attr.setAttribute("name", "android:drawable");
    attr.appendChild(graphics());

    for ( const auto& target : animations() )
        av.appendChild(target);

    return dom;
}

