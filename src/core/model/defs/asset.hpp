#pragma once

#include <unordered_set>

#include "model/reference_target.hpp"
#include "utils/pseudo_mutex.hpp"
#include "model/property/reference_property.hpp"

namespace model {

class AssetBase
{
public:
    using User = ReferencePropertyBase;

    const std::unordered_set<User*>& users() const;
    void add_user(User* user);
    void remove_user(User* user);

    void attach();
    void detach();

    /**
     * \brief Removes the asset if it isn't needed
     * \param clean_lists when \b true, remove even if the asset is in a useful list
     * \return Whether it has been removed
     */
    virtual bool remove_if_unused(bool clean_lists) = 0;

protected:
    virtual void on_users_changed() {};
    virtual model::ReferenceTarget* to_reftarget() = 0;

private:
    std::unordered_set<User*> users_;
    utils::PseudoMutex detaching;
};


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
