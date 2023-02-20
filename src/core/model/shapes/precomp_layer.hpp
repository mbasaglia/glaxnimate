/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "model/property/reference_property.hpp"
#include "model/stretchable_time.hpp"
#include "model/shapes/shape.hpp"
#include "model/assets/precomposition.hpp"


namespace glaxnimate::model {

class PreCompLayer : public ShapeElement
{
    GLAXNIMATE_OBJECT(PreCompLayer)

    GLAXNIMATE_SUBOBJECT(StretchableTime, timing)
    GLAXNIMATE_PROPERTY_REFERENCE(Precomposition, composition, &PreCompLayer::valid_precomps, &PreCompLayer::is_valid_precomp, &PreCompLayer::composition_changed)
    GLAXNIMATE_PROPERTY(QSizeF, size, {})
    GLAXNIMATE_SUBOBJECT(Transform, transform)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1, &PreCompLayer::opacity_changed, 0, 1, false, PropertyTraits::Percent)

public:
    PreCompLayer(Document* document);


    QIcon tree_icon() const override;
    QString type_name_human() const override;
    void set_time(FrameTime t) override;

    /**
     * \brief Returns the (frame) time relative to this layer
     *
     * Useful for stretching / remapping etc.
     * Always use this to get animated property values,
     * even if currently it doesn't do anything
     */
    FrameTime relative_time(FrameTime time) const;

    QRectF local_bounding_rect(FrameTime t) const override;
    QTransform local_transform_matrix(model::FrameTime t) const override;

    void add_shapes(model::FrameTime t, math::bezier::MultiBezier & bez, const QTransform& transform) const override;

    QPainterPath to_clip(model::FrameTime t) const override;

signals:
    void opacity_changed(float op);
    void composition_changed();

protected:
    QPainterPath to_painter_path_impl(model::FrameTime t) const override;
    void on_paint(QPainter*, FrameTime, PaintMode, model::Modifier*) const override;
    void on_composition_changed(model::Composition* old_comp, model::Composition* new_comp) override;

private slots:
    void on_transform_matrix_changed();


private:
    std::vector<DocumentNode*> valid_precomps() const;
    bool is_valid_precomp(DocumentNode* node) const;
    void refresh_owner_composition();

};

} // namespace glaxnimate::model
