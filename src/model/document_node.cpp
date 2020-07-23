#include "document_node.hpp"
#include "document.hpp"

model::DocumentNode::DocumentNode(Document* document)
    : document_(document)
{
    connect(this, &Object::property_changed, this, &DocumentNode::on_value_changed);
    uuid.set_value(QUuid::createUuid());
}


QString model::DocumentNode::docnode_name() const
{
    return name.get();
}

QColor model::DocumentNode::docnode_group_color() const
{
    QColor col = group_color.get();
    if ( !col.isValid() || col.alpha() == 0 )
    {
        if ( auto parent = docnode_parent() )
            return parent->docnode_group_color();
        else
            return Qt::white;
    }
    return col;
}

void model::DocumentNode::on_value_changed(const QString& name, const QVariant&)
{
    if ( name == "name" )
        emit docnode_name_changed(this->name.get());
    else if ( name == "color" )
        emit docnode_group_color_changed(this->group_color.get());
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
