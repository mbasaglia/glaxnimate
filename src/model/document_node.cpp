#include "document_node.hpp"

model::DocumentNode::DocumentNode(Document* document)
    : document_(document)
{
    connect(this, &Object::property_changed, this, &DocumentNode::on_value_changed);
}


QString model::DocumentNode::docnode_name() const
{
    if ( !name.get().isEmpty() )
        return name.get();

    QString class_name = metaObject()->className();
    int ns = class_name.lastIndexOf(":");
    if ( ns != -1 )
        class_name = class_name.mid(ns+1);
    return class_name;
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
