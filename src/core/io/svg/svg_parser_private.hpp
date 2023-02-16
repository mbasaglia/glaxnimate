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

#include "path_parser.hpp"
#include "animate_parser.hpp"
#include "math/math.hpp"
#include "font_weight.hpp"
#include "css_parser.hpp"
#include "app/utils/string_view.hpp"


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
        QSize forced_size
    ) : document(document),
        on_warning(on_warning),
        io(io),
        forced_size(forced_size)
    {
        animate_parser.fps = document ? document->main()->fps.get() : 60;
        animate_parser.on_warning = on_warning;
    }

    void load(QIODevice* device)
    {
        SvgParseError err;
        if ( !dom.setContent(device, true, &err.message, &err.line, &err.column) )
            throw err;
    }

    virtual ~SvgParserPrivate() = default;

    void parse()
    {
        size = document->size();
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

        QPointF pos;
        QVector2D scale;
        std::tie(pos, scale) = on_parse_meta(root);

        model::Layer* parent_layer = parse_objects(root);
        parent_layer->transform.get()->position.set(-pos);
        parent_layer->transform.get()->scale.set(scale);

        on_parse_finish(parent_layer, root);

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

    static qreal opacity_value(const QString& v)
    {
        if ( v.isEmpty() )
            return 1;

        if ( v.back() == '%' )
            return v.mid(0, v.size()-1).toDouble() / 100;

        return v.toDouble();
    }

private:
    void write_document_data()
    {
        document->main()->width.set(size.width());
        document->main()->height.set(size.height());

        if ( to_process < 1000 )
            document->main()->recursive_rename();

        if ( max_time <= 0 )
            max_time = 180;

        document->main()->animation->last_frame.set(max_time);
        for ( auto lay : layers )
            lay->animation->last_frame.set(max_time);
    }

    model::Layer* parse_objects(const QDomElement& root)
    {
        model::Layer* parent_layer = add_layer(&document->main()->shapes);
        parent_layer->name.set(parent_layer->type_name_human());
        parse_children({root, &parent_layer->shapes, initial_style(root), false});

        return parent_layer;
    }

protected:
    virtual void on_parse_prepare(const QDomElement& root) = 0;
    virtual QSizeF get_size(const QDomElement& root) = 0;
    virtual std::pair<QPointF, QVector2D> on_parse_meta(const QDomElement& root) = 0;
    virtual void on_parse_finish(model::Layer* parent_layer, const QDomElement& root) = 0;
    virtual void parse_shape(const ParseFuncArgs& args) = 0;
    virtual Style initial_style(const QDomElement& root) = 0;

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

    static const QRegularExpression unit_re;
};

} // namespace glaxnimate::io::svg::detail
