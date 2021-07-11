#pragma once
#ifdef _HAX_FUCKING_QMAKE_I_HATE_YOU_
    class IAlsoHateLupdate{Q_OBJECT};
#endif

#include "model/property/reference_property.hpp"
#include "model/stretchable_time.hpp"
#include "model/shapes/shape.hpp"
#include "model/assets/precomposition.hpp"


namespace model {

class PreCompLayer : public ShapeElement
{
    GLAXNIMATE_OBJECT(PreCompLayer)

    GLAXNIMATE_SUBOBJECT(model::StretchableTime, timing)
    GLAXNIMATE_PROPERTY_REFERENCE(model::Precomposition, composition, &PreCompLayer::valid_precomps, &PreCompLayer::is_valid_precomp, &PreCompLayer::composition_changed)
    GLAXNIMATE_PROPERTY(QSizeF, size, {})
    GLAXNIMATE_SUBOBJECT(model::Transform, transform)
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
    QPainterPath to_painter_path(model::FrameTime t) const override;

signals:
    void opacity_changed(float op);
    void composition_changed();

protected:
    void on_paint(QPainter*, FrameTime, PaintMode, model::Modifier*) const override;
    void removed_from_list() override;
    void added_to_list() override;

private slots:
    void on_transform_matrix_changed();


private:
    std::vector<DocumentNode*> valid_precomps() const;
    bool is_valid_precomp(DocumentNode* node) const;
    void refresh_owner_composition();

};

} // namespace model
