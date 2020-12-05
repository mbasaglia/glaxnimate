#pragma once

#include <unordered_set>

#include "model/reference_target.hpp"
#include "model/property/reference_property.hpp"

namespace model {

class Asset : public ReferenceTarget, public AssetBase
{
    Q_OBJECT

public:
    using ReferenceTarget::ReferenceTarget;

signals:
    void users_changed();

protected:
    void on_users_changed() override
    {
        emit users_changed();
    }

    model::ReferenceTarget* to_reftarget() override { return this; }
};



} // namespace model
