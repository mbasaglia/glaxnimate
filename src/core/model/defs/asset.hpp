#pragma once

#include <unordered_set>

#include "model/reference_target.hpp"
#include "utils/pseudo_mutex.hpp"
#include "model/property/reference_property.hpp"

namespace model {

class Asset : public ReferenceTarget
{
    Q_OBJECT

public:
    using User = ReferencePropertyBase;

    using ReferenceTarget::ReferenceTarget;

    const std::unordered_set<User*>& users() const;
    void add_user(User* user);
    void remove_user(User* user);

    void attach();
    void detach();

signals:
    void users_changed();

private:
    std::unordered_set<User*> users_;
    utils::PseudoMutex detaching;
};




} // namespace model
