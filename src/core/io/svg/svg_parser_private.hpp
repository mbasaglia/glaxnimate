/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

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

#include "math/math.hpp"
#include "app/utils/string_view.hpp"

#include "path_parser.hpp"
#include "animate_parser.hpp"
#include "parse_error.hpp"
#include "font_weight.hpp"
#include "css_parser.hpp"
#include "detail.hpp"


namespace glaxnimate::io::svg::detail {

class SvgParserPrivate
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


    SvgParserPrivate(
        model::Document* document,
        const std::function<void(const QString&)>& on_warning,
        ImportExport* io,
        QSize forced_size,
        model::FrameTime default_time
    ) : document(document),
        on_warning(on_warning),
        io(io),
        forced_size(forced_size),
        default_time(default_time == 0 ? 180 : default_time)
    {
        animate_parser.on_warning = on_warning;
    }

    void load(QIODevice* device)
    {
        SvgParseError err;
        if ( !dom.setContent(device, true, &err.message, &err.line, &err.column) )
            throw err;
    }

    virtual ~SvgParserPrivate() = default;

    void parse(model::Document* document = nullptr)
    {
        if ( document )
            this->document = document;

        if ( this->document->assets()->compositions->values.empty() )
            main = this->document->assets()->compositions->values.insert(std::make_unique<model::Composition>(this->document));
        else
            main = this->document->assets()->compositions->values[0];
        animate_parser.fps = main->fps.get();

        size = main->size();
        auto root = dom.documentElement();

        if ( forced_size.isValid() )
        {
            size = forced_size;
        }
        else
        {
            size = get_size(root);
        }

        to_process = 0;
        on_parse_prepare(root);
        if ( io )
            io->progress_max_changed(to_process);

        on_parse(root);

        write_document_data();
    }

protected:
    QString attr(const QDomElement& e, const QString& ns, const QString& name, const QString& defval = {})
    {
        if ( ns.isEmpty() )
            return e.attribute(name, defval);
        return e.attributeNS(xmlns.at(ns), name, defval);
    }

    qreal len_attr(const QDomElement& e, const QString& name, qreal defval = 0)
    {
        if ( e.hasAttribute(name) )
            return parse_unit(e.attribute(name));
        return defval;
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

        if ( unit == "px" || unit == "" || unit == "dp" || unit == "dip" || unit == "sp" )
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

    void mark_progress()
    {
        processed++;
        if ( io && processed % 10 == 0 )
            io->progress(processed);
    }

    QDomElement query_element(const std::vector<QString>& path, const QDomElement& parent, std::size_t index = 0)
    {
        if ( index >= path.size() )
            return parent;


        auto head = path[index];
        for ( const auto& domnode : ItemCountRange(parent.childNodes()) )
        {
            if ( domnode.isElement() )
            {
                auto child = domnode.toElement();
                if ( child.tagName() == head )
                    return query_element(path, child, index+1);
            }
        }
        return {};
    }

    QString query(const std::vector<QString>& path, const QDomElement& parent, std::size_t index = 0)
    {
        return query_element(path, parent, index).text();
    }

    std::vector<qreal> double_args(const QString& str)
    {
        auto args_s = ::utils::split_ref(str, AnimateParser::separator,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            Qt::SkipEmptyParts
#else
            QString::SkipEmptyParts
#endif
        );
        std::vector<qreal> args;
        args.reserve(args_s.size());
        std::transform(args_s.begin(), args_s.end(), std::back_inserter(args),
                        [](const ::utils::StringView& s){ return s.toDouble(); });
        return args;
    }

    // parse attributes like opacity where it's a value in [0-1] or a percentage
    static double percent_1(const QString& s)
    {
        if ( s.contains('%') )
            return ::utils::mid_ref(s, 0, s.size()-1).toDouble() / 100;
        return s.toDouble();
    }

    static model::Stroke::Cap line_cap(const QString& linecap)
    {
        if ( linecap == "round" )
            return model::Stroke::RoundCap;
        else if ( linecap == "butt" )
            return model::Stroke::ButtCap;
        else if ( linecap == "square" )
            return model::Stroke::SquareCap;

        return model::Stroke::ButtCap;
    }

    static model::Stroke::Join line_join(const QString& linecap)
    {
        if ( linecap == "round" )
            return model::Stroke::RoundJoin;
        else if ( linecap == "bevel" )
            return model::Stroke::BevelJoin;
        else if ( linecap == "miter" )
            return model::Stroke::MiterJoin;

        return model::Stroke::MiterJoin;
    }

    template<class KfCollection>
    KfCollection add_keyframes(KfCollection&& kfs)
    {
        if ( !kfs.empty() && kfs.back().time > max_time)
            max_time = kfs.back().time;

        return std::forward<KfCollection>(kfs);
    }

    void path_animation(
        const std::vector<model::Path*>& paths,
        const AnimatedProperties& anim,
        const QString& attr
    )
    {
        if ( !paths.empty() )
        {
            for ( const auto& kf : add_keyframes(anim.single(attr)) )
            {
                for ( int i = 0; i < math::min<int>(kf.values.bezier().size(), paths.size()); i++ )
                    paths[i]->shape.set_keyframe(kf.time, kf.values.bezier()[i])->set_transition(kf.transition);
            }
        }
    }

private:
    void write_document_data()
    {
        main->width.set(size.width());
        main->height.set(size.height());

        if ( max_time <= 0 )
            max_time = default_time;

        main->animation->last_frame.set(max_time);
        for ( auto lay : layers )
        {
            lay->animation->last_frame.set(max_time);
        }

        document->undo_stack().clear();
    }

protected:
    virtual void on_parse_prepare(const QDomElement& root) = 0;
    virtual QSizeF get_size(const QDomElement& root) = 0;
    virtual void parse_shape(const ParseFuncArgs& args) = 0;
    virtual void on_parse(const QDomElement& root) = 0;

    QDomDocument dom;

    qreal dpi = 96;
    QSizeF size;

    model::Document* document;

    AnimateParser animate_parser;
    model::FrameTime max_time = 0;
    std::function<void(const QString&)> on_warning;
    std::unordered_map<QString, QDomElement> map_ids;
    std::unordered_map<QString, model::BrushStyle*> brush_styles;
    std::unordered_map<QString, model::GradientColors*> gradients;
    std::vector<model::Layer*> layers;

    int to_process = 0;
    int processed = 0;
    ImportExport* io = nullptr;
    QSize forced_size;
    model::FrameTime default_time;
    model::Composition* main = nullptr;

    static const QRegularExpression unit_re;
};

} // namespace glaxnimate::io::svg::detail
