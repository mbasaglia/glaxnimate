#pragma once

#include <QRawFont>

#include "model/property/sub_object_property.hpp"
#include "shape.hpp"

namespace model {

class Font : public Object
{
    GLAXNIMATE_OBJECT(Font)
    GLAXNIMATE_PROPERTY(QString, family, "", &Font::on_family_changed, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, size, 32, &Font::on_font_changed, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(QString, style, "", &Font::on_font_changed, {}, PropertyTraits::Visual)

public:
    explicit Font(Document* doc);

    const QRawFont& raw_font() const;
    const QFont& query() const;
    const QStringList& styles() const;
    QString type_name_human() const override;

private:
    void on_family_changed();
    void on_font_changed();
    bool valid_style(const QString& style);
    void refresh_styles();

    QStringList styles_;
    QFont query_;
    QRawFont raw_;
};

class TextShape : public ShapeElement
{
    GLAXNIMATE_OBJECT(TextShape)
    GLAXNIMATE_PROPERTY(QString, text, {}, {}, PropertyTraits::Visual)
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
