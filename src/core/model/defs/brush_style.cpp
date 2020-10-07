#include "brush_style.hpp"

QIcon model::BrushStyle::reftarget_icon() const
{
    if ( icon.isNull() )
    {
        icon = QPixmap(32, 32);
        fill_icon(icon);
    }
    return icon;
}

void model::BrushStyle::add_user(model::BrushStyle::User* user)
{
    if ( !detaching )
        users_.insert(user);
}

void model::BrushStyle::remove_user(model::BrushStyle::User* user)
{
    if ( !detaching )
        users_.erase(user);
}

const std::unordered_set<model::BrushStyle::User*> & model::BrushStyle::users() const
{
    return users_;
}

void model::BrushStyle::attach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set(this);
    }
}

void model::BrushStyle::detach()
{
    if ( auto lock = detaching.get_lock() )
    {
        for ( auto user : users_ )
            user->set(nullptr);
    }
}

QString model::BrushStyle::object_name() const
{
    if ( name.get().isEmpty() )
        return type_name_human();
    return name.get();
}
