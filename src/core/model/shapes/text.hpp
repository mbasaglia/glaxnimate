#pragma once

#include <QRawFont>
#include <QFontMetricsF>

#include "model/property/sub_object_property.hpp"
#include "model/property/option_list_property.hpp"
#include "shape.hpp"

namespace model {

class Font : public Object
{
    GLAXNIMATE_OBJECT(Font)
    GLAXNIMATE_PROPERTY_OPTIONS(QString, family, "", QStringList,
        &Font::families, &Font::on_family_changed, {}, PropertyTraits::Visual, OptionListPropertyBase::FontCombo)
    GLAXNIMATE_PROPERTY_OPTIONS(float, size, 32, QList<int>,
        &Font::standard_sizes, &Font::on_font_changed, {}, PropertyTraits::Visual, OptionListPropertyBase::LaxValues)
    GLAXNIMATE_PROPERTY_OPTIONS(QString, style, "", QStringList,
        &Font::styles, &Font::on_font_changed, &Font::valid_style, PropertyTraits::Visual)

public:
    struct CharData
    {
        quint32 glyph;
        QPointF advance;
        QPainterPath path;
    };

    using CharDataCache = std::unordered_map<quint32, QPainterPath>;

    explicit Font(Document* doc);
    ~Font();

    const QRawFont& raw_font() const;
    const QFont& query() const;
    const QFontMetricsF& metrics() const;

    QStringList styles() const;
    QStringList families() const;
    QList<int> standard_sizes() const;

    QString type_name_human() const override;

    std::vector<CharData> line_data(const QString& line, CharDataCache& cache) const;
    qreal line_spacing() const;

private:
    void on_family_changed();
    void on_font_changed();
    bool valid_style(const QString& style);

    class Private;
    std::unique_ptr<Private> d;
};

class TextShape : public ShapeElement
{
    GLAXNIMATE_OBJECT(TextShape)
    GLAXNIMATE_PROPERTY(QString, text, {}, {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_SUBOBJECT(Font, font)

public:
    using ShapeElement::ShapeElement;
    void add_shapes(FrameTime t, math::bezier::MultiBezier& bez) const override;
    QPainterPath to_painter_path(FrameTime t) const override;
    QIcon tree_icon() const override;
    QRectF local_bounding_rect(FrameTime t) const override;
    QString type_name_human() const override;
};

} // namespace model
