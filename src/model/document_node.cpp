#include "document_node.hpp"
#include "document.hpp"

model::DocumentNode::DocumentNode(Document* document)
    : Object(document)
{
    group_icon.fill(Qt::white);
    uuid.set_value(QUuid::createUuid());
}


QString model::DocumentNode::docnode_name() const
{
    return name.get();
}

QColor model::DocumentNode::docnode_group_color() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_group_parent() )
            return parent->docnode_group_color();
        else
            return Qt::white;
    }
    return group_color.get();
}

void model::DocumentNode::on_property_changed(const QString& name, const QVariant&)
{
    if ( name == "name" )
    {
        emit docnode_name_changed(this->name.get());
    }
    else if ( name == "color" )
    {
        if ( docnode_valid_color() )
            group_icon.fill(group_color.get());
        else
            group_icon.fill(Qt::white);
        docnode_on_update_group(true);
    }
}

QString model::DocumentNode::object_name() const
{
    if ( name.get().isEmpty() )
        return type_name();
    return name.get();
}


bool model::DocumentNode::docnode_is_instance(const QString& type_name) const
{
    if ( type_name.isEmpty() )
        return true;

    for ( const QMetaObject* meta = metaObject(); meta; meta = meta->superClass() )
    {
        if ( naked_type_name(meta->className()) == type_name )
            return true;
    }

    return false;
}

void model::DocumentNode::docnode_on_update_group(bool force)
{
    if ( force || docnode_valid_color() )
    {
        emit docnode_group_color_changed(this->group_color.get());
        for ( const auto& gc : docnode_group_children() )
            gc->docnode_on_update_group();
    }
}

bool model::DocumentNode::docnode_valid_color() const
{
    QColor col = group_color.get();
    return col.isValid() && col.alpha() > 0;
}

const QPixmap & model::DocumentNode::docnode_group_icon() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_group_parent() )
            return parent->docnode_group_icon();
    }

    return group_icon;
}

