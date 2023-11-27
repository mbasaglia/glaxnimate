/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QRawFont>
#include <QFontMetricsF>

#include "model/property/sub_object_property.hpp"
#include "model/property/option_list_property.hpp"
#include "model/property/reference_property.hpp"
#include "shape.hpp"

namespace glaxnimate::model {

class Font : public Object
{
    GLAXNIMATE_OBJECT(Font)
    GLAXNIMATE_PROPERTY_OPTIONS(QString, family, {}, QStringList,
        &Font::families, &Font::on_family_changed, {}, PropertyTraits::Visual, OptionListPropertyBase::FontCombo)
    GLAXNIMATE_PROPERTY_OPTIONS(float, size, 32, QList<int>,
        &Font::standard_sizes, &Font::on_font_changed, {}, PropertyTraits::Visual, OptionListPropertyBase::LaxValues)
    GLAXNIMATE_PROPERTY_OPTIONS(QString, style, {}, QStringList,
        &Font::styles, &Font::on_font_changed, &Font::valid_style, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, line_height, 1, &Font::on_font_changed, {}, PropertyTraits::Visual|PropertyTraits::Percent)

public:
    struct CharData
    {
        quint32 glyph;
        QPointF position;
    };

    struct LineData
    {
        std::vector<CharData> glyphs;
        QRectF bounds;
        QPointF baseline;
        QPointF advance;
        QString text;
    };

    using ParagraphData = std::vector<LineData>;

    using CharDataCache = std::unordered_map<quint32, QPainterPath>;

    explicit Font(Document* doc);
    ~Font();

    const QRawFont& raw_font() const;
    const QFont& query() const;
    const QFontMetricsF& metrics() const;

    void from_qfont(const QFont& f);

    QStringList styles() const;
    QStringList families() const;
    QList<int> standard_sizes() const;

    QString type_name_human() const override;

    ParagraphData layout(const QString& string) const;

    /**
     * \brief Distance between two baselines
     */
    qreal line_spacing() const;

    /**
     * \brief Distance between two baselines for a line_height of 1
     */
    qreal line_spacing_unscaled() const;

    QPainterPath path_for_glyph(quint32 glyph, CharDataCache& cache, bool fix_paint) const;

Q_SIGNALS:
    void font_changed();

protected:
    void on_transfer(model::Document* doc) override;

private:
    void on_family_changed();
    void on_font_changed();
    void refresh_data(bool update_styles);
    bool valid_style(const QString& style);

    class Private;
    std::unique_ptr<Private> d;
};

class TextShape : public ShapeElement
{
    GLAXNIMATE_OBJECT(TextShape)
    GLAXNIMATE_PROPERTY(QString, text, {}, &TextShape::on_text_changed, {}, PropertyTraits::Visual)
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
    GLAXNIMATE_SUBOBJECT(Font, font)
    GLAXNIMATE_PROPERTY_REFERENCE(model::ShapeElement, path, &TextShape::valid_paths, &TextShape::is_valid_path, &TextShape::path_changed)
    GLAXNIMATE_ANIMATABLE(float, path_offset, 0, &TextShape::on_text_changed)

public:
    explicit TextShape(model::Document* document);

    void add_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const override;

    QPainterPath shape_data(FrameTime t) const;

    QIcon tree_icon() const override;
    QRectF local_bounding_rect(FrameTime t) const override;
    QString type_name_human() const override;
    std::unique_ptr<ShapeElement> to_path() const override;

    /**
     * \brief Position where the next character would be
     *
     * ie: where to add another TextShape to make them flow together
     */
    QPointF offset_to_next_character() const;

protected:
    QPainterPath to_painter_path_impl(FrameTime t) const override;

private:
    void on_font_changed();
    void on_text_changed();
    const QPainterPath& untranslated_path(FrameTime t) const;

    std::vector<DocumentNode*> valid_paths() const;
    bool is_valid_path(DocumentNode* node) const;
    void path_changed(model::ShapeElement* new_path, model::ShapeElement* old_path);

    mutable Font::CharDataCache cache;
    mutable QPainterPath shape_cache;
};

} // namespace glaxnimate::model
