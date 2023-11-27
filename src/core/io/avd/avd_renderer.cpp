/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "avd_renderer.hpp"

#include "io/svg/detail.hpp"
#include "io/svg/svg_renderer.hpp"

#include "model/document.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/trim.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/path.hpp"
#include "model/animation/join_animatables.hpp"

#include <vector>
#include <variant>

class glaxnimate::io::avd::AvdRenderer::Private
{
public:
    using PropRet = std::vector<std::pair<QString, QString>>;

    struct Keyframe
    {
        QString value;
        // TODO interpolators
    };

    class AnimationHelper
    {
    public:
        Private* parent = nullptr;
        QString name;
        std::map<QString, std::map<qreal, Keyframe>> keyframes = {};

        bool animated() const
        {
            return !keyframes.empty();
        }

        static QString attr(const QString& name)
        {
            return "android:"_qs + name;
        }

        qreal frame_to_ms(model::FrameTime f) const
        {
            return f * 1000 / parent->fps;
        }

        template<class Callback>
        void render_properties(
            QDomElement& element,
            std::vector<const model::AnimatableBase*> properties,
            const Callback& callback
        )
        {
            model::JoinAnimatables j(std::move(properties), model::JoinAnimatables::Normal);

            auto xml_values = callback(j.current_value());
            for ( const auto& p : xml_values )
                element.setAttribute(attr(p.first), p.second);

            if ( j.animated() )
            {
                for ( const auto& kf : j )
                {
                    xml_values = callback(kf.values);
                    for ( const auto& p : xml_values )
                    {
                        keyframes[p.first][frame_to_ms(kf.time)] = Keyframe{p.second};
                    }
                }
            }
        }

        /// \todo Option for propertyValuesHolder+keyframe?
        QDomElement render_object_animators() const
        {
            QDomElement target = parent->dom.createElement("target"_qs);
            target.setAttribute("android:name"_qs, name);
            QDomElement attr = parent->dom.createElement("aapt:attr"_qs);
            target.appendChild(attr);
            attr.setAttribute("name"_qs, "android:animation"_qs);
            QDomElement set = parent->dom.createElement("set"_qs);
            attr.appendChild(set);

            for ( const auto& prop : keyframes )
            {
                QString type;
                if ( prop.first == "pathData"_qs )
                    type = "pathType"_qs;
                else if ( prop.first.contains("Color"_qs) )
                    type = "colorType"_qs;
                else
                    type = "floatType"_qs;

                auto iter = prop.second.begin();

                while ( iter != prop.second.end() )
                {
                    auto start = iter->first;
                    QDomElement anim = parent->dom.createElement("objectAnimator"_qs);
                    anim.setAttribute("android:propertyName"_qs, prop.first);
                    anim.setAttribute("android:valueType"_qs, type);
                    anim.setAttribute("android:startOffset"_qs, QString::number(start));
                    anim.setAttribute("android:valueFrom"_qs, iter->second.value);

                    ++iter;
                    if ( iter == prop.second.end() )
                        break;

                    anim.setAttribute("android:valueTo"_qs, iter->second.value);
                    anim.setAttribute("android:duration"_qs, QString::number(iter->first - start));
                    set.appendChild(anim);
                }
            }

            return target;
        }
    };

    void render(model::Composition* comp)
    {
        fps = comp->fps.get();
        vector = dom.createElement("vector"_qs);
        vector.setAttribute("android:width"_qs, QStringLiteral("%1dp").arg(comp->width.get()));
        vector.setAttribute("android:height"_qs, QStringLiteral("%1dp").arg(comp->height.get()));
        vector.setAttribute("android:viewportWidth"_qs, QString::number(comp->width.get()));
        vector.setAttribute("android:viewportHeight"_qs, QString::number(comp->height.get()));

        render_comp(comp, vector);
    }

    void render_comp(model::Composition* comp, QDomElement& parent)
    {
        parent.setAttribute("android:name"_qs, unique_name(comp, false));
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
            QDomElement group = dom.createElement("group"_qs);
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
            base = "item_"_qs + node->uuid.get().toString(QUuid::Id128);


        QString name = base;
        if ( is_duplicate )
            name += "_"_qs + QString::number(unique_id++);

        while ( names.count(name) )
        {
            name = base + "_"_qs + QString::number(unique_id++);
        }

        names.insert(name);
        return name;
    }

    QDomElement render_group(model::Group* group, QDomElement& parent)
    {
        QDomElement elm = dom.createElement("group"_qs);
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


    void render_shapes(
        const std::vector<model::Shape*>& shapes,
        const QString& name,
        QDomElement& parent,
        model::Fill* fill,
        model::Stroke* stroke,
        model::Trim* trim
    )
    {
        if ( shapes.empty() )
            return;

        auto path = dom.createElement("path"_qs);
        parent.appendChild(path);
        path.setAttribute("android:name"_qs, name);
        render_shapes_to_path_data(shapes, name, path);
        render_fill(fill, name, path);
        render_stroke(stroke, name, path);
        render_trim(trim, name, path);
    }

    void render_shapes_to_path_data(const std::vector<model::Shape*>& shapes, const QString& name, QDomElement& elem)
    {
        std::vector<std::unique_ptr<model::ShapeElement>> saved;
        std::vector<const model::AnimatableBase*> paths;
        paths.reserve(shapes.size());

        for ( const auto& sh : shapes )
        {
            if ( auto p = sh->cast<model::Path>() )
            {
                paths.push_back(&p->shape);
            }
            else
            {
                auto conv = sh->to_path();
                collect_paths(conv.get(), paths);
                saved.push_back(std::move(conv));
            }
        }

        auto& anim = animator(name);
        anim.render_properties(elem, paths, [](const std::vector<QVariant>& v) -> PropRet {
            return {
                {"pathData"_qs, paths_to_path_data(v)},
            };
        });
    }

    void collect_paths(model::ShapeElement* element, std::vector<const model::AnimatableBase*>& paths)
    {
        if ( auto p = element->cast<model::Path>() )
        {
            paths.push_back(&p->shape);
        }
        else if ( auto g = element->cast<model::Group>() )
        {
            for ( const auto& c : g->shapes )
                collect_paths(c.get(), paths);
        }
    }

    static QString paths_to_path_data(const std::vector<QVariant>& paths)
    {
        math::bezier::MultiBezier bez;
        for ( const auto& path : paths )
            bez.beziers().push_back(path.value<math::bezier::Bezier>());

        return svg::path_data(bez).first;
    }

    void render_fill(model::Fill* fill, const QString& name, QDomElement& element)
    {
        if ( !fill )
            return;

        render_styler_color(fill, name, "fillColor"_qs, element);

        auto& anim = animator(name);
        anim.render_properties(element, {&fill->opacity}, [](const std::vector<QVariant>& v) -> PropRet {
            return {
                {"fillAlpha"_qs, QString::number(v[0].toDouble())},
            };
        });
        element.setAttribute("android:fillType"_qs, fill->fill_rule.get() == model::Fill::EvenOdd ? "evenOdd"_qs : "nonZero"_qs);

    }

    void render_stroke(model::Stroke* stroke, const QString& name, QDomElement& element)
    {
        if ( !stroke )
            return;

        render_styler_color(stroke, name, "strokeColor"_qs, element);

        auto& anim = animator(name);
        anim.render_properties(element, {&stroke->opacity}, [](const std::vector<QVariant>& v) -> PropRet {
            return {
                {"strokeAlpha"_qs, QString::number(v[0].toDouble())},
            };
        });
        anim.render_properties(element, {&stroke->width}, [](const std::vector<QVariant>& v) -> PropRet {
            return {
                {"strokeWidth"_qs, QString::number(v[0].toDouble())},
            };
        });

        element.setAttribute("android:strokeWidth"_qs, QString::number(stroke->width.get()));
        element.setAttribute("android:strokeMiterLimit"_qs, QString::number(stroke->miter_limit.get()));
        switch ( stroke->cap.get() )
        {
            case model::Stroke::RoundCap:
                element.setAttribute("android:strokeLineCap"_qs, "round"_qs);
                break;
            case model::Stroke::ButtCap:
                element.setAttribute("android:strokeLineCap"_qs, "butt"_qs);
                break;
            case model::Stroke::SquareCap:
                element.setAttribute("android:strokeLineCap"_qs, "square"_qs);
                break;
        }
        switch ( stroke->join.get() )
        {
            case model::Stroke::RoundJoin:
                element.setAttribute("android:strokeLineJoin"_qs, "round"_qs);
                break;
            case model::Stroke::MiterJoin:
                element.setAttribute("android:strokeLineJoin"_qs, "miter"_qs);
                break;
            case model::Stroke::BevelJoin:
                element.setAttribute("android:strokeLineJoin"_qs, "bevel"_qs);
                break;
        }
    }

    void render_styler_color(model::Styler* styler, const QString& name, const QString& attr, QDomElement& element)
    {
        auto use = styler->use.get();

        if ( auto color = use->cast<model::NamedColor>() )
        {
            auto& anim = animator(name);
            anim.render_properties(element, {&color->color}, [&attr](const std::vector<QVariant>& v) -> PropRet { return {
                {attr, render_color(v[0].value<QColor>())},
            };});
        }
        else if ( auto gradient = use->cast<model::Gradient>() )
        {
            render_gradient(attr, gradient, element);
        }
        else
        {
            auto& anim = animator(name);
            anim.render_properties(element, {&styler->color}, [&attr](const std::vector<QVariant>& v) -> PropRet { return {
                {attr, render_color(v[0].value<QColor>())},
            };});
        }
    }

    void render_gradient(const QString& attr_name, model::Gradient* gradient, QDomElement& element)
    {
        auto attr = dom.createElement("aapt:attr"_qs);
        attr.setAttribute("name"_qs, "android:"_qs + attr_name);
        element.appendChild(attr);
        auto gradel = dom.createElement("gradient"_qs);
        attr.appendChild(gradel);
        switch ( gradient->type.get() )
        {
            case model::Gradient::Linear:
                gradel.setAttribute("android:type"_qs, "linear"_qs);
                break;
            case model::Gradient::Radial:
                gradel.setAttribute("android:type"_qs, "radial"_qs);
                break;
            case model::Gradient::Conical:
                gradel.setAttribute("android:type"_qs, "sweep"_qs);
                break;
        }

        gradel.setAttribute("startX"_qs, gradient->start_point.get().x());
        gradel.setAttribute("startY"_qs, gradient->start_point.get().y());
        gradel.setAttribute("endX"_qs, gradient->end_point.get().x());
        gradel.setAttribute("endY"_qs, gradient->end_point.get().y());

        if ( auto cols = gradient->colors.get() )
        {
            for ( const auto& stop : cols->colors.get() )
            {
                auto item = dom.createElement("item"_qs);
                item.setAttribute("android:color"_qs, render_color(stop.second));
                item.setAttribute("android:offset"_qs, QString::number(stop.first));
            }
        }
    }

    void render_trim(model::Trim* trim, const QString& name, QDomElement& element)
    {
        if ( !trim )
            return;

        auto& anim = animator(name);
        anim.render_properties(element, {&trim->start}, [](const std::vector<QVariant>& v) -> PropRet { return {
            {"trimPathStart"_qs, QString::number(v[0].toDouble())},
        };});
        anim.render_properties(element, {&trim->end}, [](const std::vector<QVariant>& v) -> PropRet { return {
            {"trimPathEnd"_qs, QString::number(v[0].toDouble())},
        };});
        anim.render_properties(element, {&trim->offset}, [](const std::vector<QVariant>& v) -> PropRet { return {
            {"trimPathOffset"_qs, QString::number(v[0].toDouble())},
        };});
    }

    QDomElement render_clip_path(model::ShapeElement* element)
    {
        QDomElement clip = dom.createElement("clip-path"_qs);
        QString name = unique_name(element, false);
        clip.setAttribute("android:name"_qs, name);

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

    static QString color_comp(int comp)
    {
        return QString::number(comp, 16).rightJustified(2, '0'_qc);
    }

    static QString render_color(const QColor& color)
    {
        return "#"_qs + color_comp(color.alpha()) + color_comp(color.red()) + color_comp(color.green()) + color_comp(color.blue());
    }

    void render_transform(model::Transform* trans, QDomElement& elm, const QString& name)
    {
        auto& anim = animator(name);
        anim.render_properties(elm, {&trans->anchor_point, &trans->position}, [](const std::vector<QVariant>& v) -> PropRet {
            auto ap = v[0].toPointF();
            auto pos = v[1].toPointF() - ap;
            return {
                {"pivotX"_qs, QString::number(ap.x())},
                {"pivotY"_qs, QString::number(ap.y())},
                {"translateX"_qs, QString::number(pos.x())},
                {"translateY"_qs, QString::number(pos.y())},
            };
        });
        anim.render_properties(elm, {&trans->scale}, [](const std::vector<QVariant>& v) -> PropRet {
            auto scale = v[0].value<QVector2D>();
            return {
                {"scaleX"_qs, QString::number(scale.x())},
                {"scaleY"_qs, QString::number(scale.y())},
            };
        });
        anim.render_properties(elm, {&trans->rotation}, [](const std::vector<QVariant>& v) -> PropRet {
            return {
                {"rotation"_qs, QString::number(v[0].toDouble())},
            };
        });
    }

    void render_anim(QDomElement& container)
    {
        for ( const auto& p : animations )
        {
            if ( p.second.animated() )
                container.appendChild(p.second.render_object_animators());
        }
    }

    AnimationHelper& animator(const QString& name)
    {
        auto iter = animations.find(name);
        if ( iter == animations.end() )
            iter = animations.insert({name, {this, name}}).first;
        return iter->second;
    }

    int fps = 60;
    int unique_id = 0;
    QDomDocument dom;
    QDomElement vector;
    std::map<QString, AnimationHelper> animations;
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

void glaxnimate::io::avd::AvdRenderer::render(model::Composition* comp)
{
    d->render( comp);
}

QDomElement glaxnimate::io::avd::AvdRenderer::graphics()
{
    return d->vector;
}


QDomDocument glaxnimate::io::avd::AvdRenderer::single_file()
{
    QDomDocument dom;
    auto av = dom.createElement("animated-vector"_qs);
    dom.appendChild(av);
    av.setAttribute("xmlns"_qs, svg::detail::xmlns.at("android"_qs));
    for ( const auto& p : svg::detail::xmlns )
    {
        if ( p.second.contains("android"_qs) )
            av.setAttribute("xmlns:"_qs + p.first, p.second);
    }

    auto attr = dom.createElement("aapt:attr"_qs);
    av.appendChild(attr);
    attr.setAttribute("name"_qs, "android:drawable"_qs);
    attr.appendChild(graphics());

    d->render_anim(av);

    return dom;
}

