#pragma once
#include "asset.hpp"
#include "model/composition.hpp"

namespace model {

class Precomposition : public Composition, public AssetBase
{
    GLAXNIMATE_OBJECT(Precomposition)

public:
    using Composition::Composition;

    QIcon docnode_icon() const override;
    QString type_name_human() const override;
    QRectF local_bounding_rect(FrameTime) const override;
    bool remove_if_unused(bool clean_lists) override;

protected:
    model::ReferenceTarget* to_reftarget() override { return this; }
};

} // namespace model

