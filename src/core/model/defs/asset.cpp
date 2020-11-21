#include "asset.hpp"

void model::AssetBase::add_user(model::AssetBase::User* user)
{
    if ( !detaching )
    {
        users_.insert(user);
        on_users_changed();
    }
}

void model::AssetBase::remove_user(model::AssetBase::User* user)
{
    if ( !detaching )
    {
        users_.erase(user);
        on_users_changed();
    }
}

const std::unordered_set<model::AssetBase::User*> & model::AssetBase::users() const
{
    return users_;
}

void model::AssetBase::attach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set_ref(to_reftarget());
    }
}

void model::AssetBase::detach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set_ref(nullptr);
    }
}
