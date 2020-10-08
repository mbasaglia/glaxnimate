#include "def.hpp"

void model::Def::add_user(model::Def::User* user)
{
    if ( !detaching )
    {
        users_.insert(user);
        emit users_changed();
    }
}

void model::Def::remove_user(model::Def::User* user)
{
    if ( !detaching )
    {
        users_.erase(user);
        emit users_changed();
    }
}

const std::unordered_set<model::Def::User*> & model::Def::users() const
{
    return users_;
}

void model::Def::attach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set_ref(this);
    }
}

void model::Def::detach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set_ref(nullptr);
    }
}
