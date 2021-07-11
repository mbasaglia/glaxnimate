#pragma once
#ifdef _HAX_FUCKING_QMAKE_I_HATE_YOU_
    class IAlsoHateLupdate{Q_OBJECT};
#endif
#include "asset.hpp"
#include "model/composition.hpp"

namespace model {

class Precomposition : public Composition, public AssetBase
{
    GLAXNIMATE_OBJECT(Precomposition)

public:
    using Composition::Composition;

    QIcon tree_icon() const override;
    QString type_name_human() const override;
    QRectF local_bounding_rect(FrameTime) const override;
    bool remove_if_unused(bool clean_lists) override;
    DocumentNode* docnode_parent() const override;

};

} // namespace model

