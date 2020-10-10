#include "asset.hpp"

void model::Asset::add_user(model::Asset::User* user)
{
    if ( !detaching )
    {
        users_.insert(user);
        emit users_changed();
    }
}

void model::Asset::remove_user(model::Asset::User* user)
{
    if ( !detaching )
    {
        users_.erase(user);
        emit users_changed();
    }
}

const std::unordered_set<model::Asset::User*> & model::Asset::users() const
{
    return users_;
}

void model::Asset::attach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set_ref(this);
    }
}

void model::Asset::detach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set_ref(nullptr);
    }
}
