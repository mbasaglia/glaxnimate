#include "property_model_private.hpp"
#include "model/assets/assets.hpp"


void item_models::PropertyModelBase::Private::begin_insert_row(item_models::PropertyModelBase::Private::Subtree* row_tree, int index)
{
    model->beginInsertRows(subtree_index(row_tree), index, index);
}

void item_models::PropertyModelBase::Private::end_insert_row()
{
    model->endInsertRows();
}

void item_models::PropertyModelBase::Private::add_object(model::Object* object, Subtree* parent, bool insert_row, int index)
{
    auto& container = parent ? parent->children : roots;
    if ( std::find_if(container.begin(), container.end(), [object](Subtree* st){ return st->object == object; }) != container.end() )
        return;

    if ( index == -1 )
        index = container.size();

    if ( insert_row )
        begin_insert_row(parent, index);

    auto node = do_add_node(Subtree{object, parent ? parent->id : 0}, parent, index);
    connect_recursive(node, insert_row);

    if ( insert_row )
        end_insert_row();
}

item_models::PropertyModelBase::Private::Subtree* item_models::PropertyModelBase::Private::do_add_node(Subtree st, Subtree* parent, int index)
{
    auto it = nodes.insert({next_id, st}).first;


    auto& container = parent ? parent->children : roots;
    if ( index == -1 )
        container.push_back(&it->second);
    else
        container.insert(container.begin() + index, &it->second);

    it->second.id = next_id;
    next_id++;
    return &it->second;
}

item_models::PropertyModelBase::Private::Subtree* item_models::PropertyModelBase::Private::add_node(Subtree st)
{
    auto parent = node(st.parent);
    return do_add_node(std::move(st), parent, -1);
}

void item_models::PropertyModelBase::Private::clear()
{
    for ( const auto& p : roots )
        disconnect_recursive(p);

    roots.clear();
    next_id = 1;
    nodes.clear();
    objects.clear();
}

item_models::PropertyModelBase::Private::Subtree* item_models::PropertyModelBase::Private::node_from_index(const QModelIndex& index)
{
    if ( !index.isValid() )
        return nullptr;

    auto it = nodes.find(index.internalId());
    return it == nodes.end() ? nullptr : &it->second;
}

item_models::PropertyModelBase::Private::Subtree* item_models::PropertyModelBase::Private::node(id_type id)
{
    auto it = nodes.find(id);
    return it != nodes.end() ? &it->second : nullptr;
}

QVariant item_models::PropertyModelBase::Private::data_name(Subtree* tree, int role)
{
    if ( role == Qt::DisplayRole )
    {
        if ( tree->prop_index != -1 )
            return tree->prop_index;
        else if ( tree->prop )
            return tree->prop->name();
        else if ( tree->object )
            return tree->object->object_name();
    }
    else if ( role == Qt::FontRole )
    {
        QFont font;
        font.setBold(true);
        return font;
    }
    else if ( role == Qt::EditRole && tree->visual_node )
    {
        return tree->visual_node->object_name();
    }
    else if ( role == Qt::DecorationRole && tree->visual_node )
    {
        return tree->visual_node->tree_icon();
    }


    return {};
}

QVariant item_models::PropertyModelBase::Private::data_value(Subtree* tree, int role)
{
    if ( !tree->prop )
    {
        return {};
    }

    model::BaseProperty* prop = tree->prop;
    model::PropertyTraits traits = prop->traits();

    if ( role == Qt::ForegroundRole )
    {
        if ( (traits.flags & (model::PropertyTraits::List|model::PropertyTraits::ReadOnly))
            || traits.type == model::PropertyTraits::Object || traits.type == model::PropertyTraits::Unknown
        )
            return QApplication::palette().color(QPalette::Disabled, QPalette::Text);
    }

    if ( role == Flags )
        return tree->prop->traits().flags;

    if ( role == ReferenceProperty && tree->prop->traits().flags & model::PropertyTraits::OptionList )
        return QVariant::fromValue(static_cast<model::OptionListPropertyBase*>(tree->prop));

    if ( (traits.flags & model::PropertyTraits::Animated) )
    {
        model::AnimatableBase* anprop = static_cast<model::AnimatableBase*>(prop);
        auto frame_status = anprop->keyframe_status(document->current_time());

        if ( role == Qt::DecorationRole )
        {
            switch ( frame_status )
            {
                case model::AnimatableBase::Tween:
                    return QIcon(app::Application::instance()->data_file("images/keyframe/status/tween.svg"));
                case model::AnimatableBase::IsKeyframe:
                    return QIcon(app::Application::instance()->data_file("images/keyframe/status/key.svg"));
                case model::AnimatableBase::Mismatch:
                    return QIcon(app::Application::instance()->data_file("images/keyframe/status/mismatch.svg"));
                case model::AnimatableBase::NotAnimated:
                    return QIcon(app::Application::instance()->data_file("images/keyframe/status/not-animated.svg"));
            }

        }
        else if ( role == Qt::BackgroundRole )
        {
            switch ( frame_status )
            {
                case model::AnimatableBase::Tween:
                    return QColor::fromHsv(100, 167, 127);
                case model::AnimatableBase::IsKeyframe:
                    return QColor::fromHsv(51, 171, 133);
                case model::AnimatableBase::Mismatch:
                    return QColor::fromHsv(29, 180, 149);
                case model::AnimatableBase::NotAnimated:
                    return QColor::fromHsv(0, 0, 120);
            }
        }
        else if ( role == Qt::ForegroundRole )
        {
            return QColor(Qt::white);
        }
        else if ( role == MinValue && traits.type == model::PropertyTraits::Float )
        {
            return static_cast<model::AnimatedProperty<float>*>(anprop)->min();
        }
        else if ( role == MaxValue && traits.type == model::PropertyTraits::Float )
        {
            return static_cast<model::AnimatedProperty<float>*>(anprop)->max();
        }
    }

    if ( (traits.flags & model::PropertyTraits::List) || traits.type == model::PropertyTraits::Unknown )
    {
        return {};
    }
    else if ( traits.type == model::PropertyTraits::Object )
    {
        if ( tree->object && role == Qt::DisplayRole )
            return tree->object->object_name();
        return {};
    }
    else if ( traits.type == model::PropertyTraits::Bool )
    {
        if ( role == Qt::CheckStateRole )
            return QVariant::fromValue(prop->value().toBool() ? Qt::Checked : Qt::Unchecked);
        return {};
    }
    else if ( traits.type == model::PropertyTraits::ObjectReference )
    {
        if ( role == Qt::DisplayRole )
        {
            QVariant value = prop->value();
            if ( value.isNull() )
                return "";
            return value.value<model::DocumentNode*>()->object_name();
        }

        if ( role == Qt::DecorationRole )
        {
            QVariant value = prop->value();
            if ( value.isNull() )
                return {};
            return QIcon(value.value<model::DocumentNode*>()->instance_icon());
        }

        if ( role == ReferenceProperty )
            return QVariant::fromValue(static_cast<model::ReferencePropertyBase*>(tree->prop));

        return {};
    }
    else if ( traits.type == model::PropertyTraits::Enum )
    {
        if ( role == Qt::DisplayRole )
            return EnumCombo::data_for(prop->value()).first;
        if ( role == Qt::EditRole )
            return prop->value();
        if ( role == Qt::DecorationRole )
            return QIcon::fromTheme(EnumCombo::data_for(prop->value()).second);
        return {};
    }
    else
    {
        if ( role == Qt::DisplayRole && (prop->traits().flags & model::PropertyTraits::Percent) )
            return QString(tr("%1%").arg(prop->value().toDouble() * 100));
        if ( role == Qt::DisplayRole || role == Qt::EditRole )
            return prop->value();
        return {};
    }
}

void item_models::PropertyModelBase::Private::connect_recursive(Subtree* this_node, bool insert_row)
{
    auto object = this_node->object;
    if ( !object )
        return;

    objects[object] = this_node->id;
    QObject::connect(object, &model::Object::destroyed, model, &PropertyModelBase::on_delete_object);
    QObject::connect(object, &model::Object::removed_from_list, model, &PropertyModelBase::on_delete_object);
    QObject::connect(object, &model::Object::property_changed, model, &PropertyModelBase::property_changed);

    on_connect(object, this_node, insert_row, nullptr);
}

void item_models::PropertyModelBase::Private::connect_subobject(model::Object* object, Subtree* this_node, bool insert_row)
{
    this_node->expand_referenced = true;

    if ( !object )
        return;

    QObject::connect(object, &model::Object::property_changed, model, &PropertyModelBase::property_changed);

    ReferencedPropertiesMap* referenced = nullptr;
    if ( this_node->prop && this_node->prop->traits().type == model::PropertyTraits::ObjectReference )
        referenced = &referenced_properties[object];

    on_connect(object, this_node, insert_row, referenced);
}


void item_models::PropertyModelBase::Private::disconnect_recursive(Subtree* node)
{
    if ( node->object )
    {
        QObject::disconnect(node->object, nullptr, model, nullptr);
        objects.erase(node->object);
    }

    for ( Subtree* child : node->children )
    {
        disconnect_recursive(child);
        nodes.erase(child->id);
    }

    node->children.clear();
}

item_models::PropertyModelBase::Private::Subtree* item_models::PropertyModelBase::Private::object_tree(model::Object* obj)
{
    auto it1 = objects.find(obj);
    if ( it1 == objects.end() )
        return nullptr;

    auto it2 = nodes.find(it1->second);
    if ( it2 == nodes.end() )
        return nullptr;

    return &it2->second;
}

void item_models::PropertyModelBase::Private::on_delete_object(model::Object* obj)
{
    auto it = objects.find(obj);
    if ( it == objects.end() )
        return;

    auto it2 = nodes.find(it->second);
    if ( it2 == nodes.end() )
        return;

    Subtree* node = &it2->second;

    auto index = model->object_index(obj);
    model->beginRemoveRows(index.parent(), index.row(), index.row());

    for ( Subtree* child : node->children )
    {
        disconnect_recursive(child);
        nodes.erase(child->id);
    }

    if ( node->parent )
    {
        auto& siblings = this->node(node->parent)->children;
        for ( auto itc = siblings.begin(); itc != siblings.end(); ++itc )
        {
            if ( *itc == node )
            {
                siblings.erase(itc);
                break;
            }
        }
    }

    auto it_roots = std::find(roots.begin(), roots.end(), node);
    if ( it_roots != roots.end() )
        roots.erase(it_roots);

    objects.erase(it);
    nodes.erase(it2);

    model->endRemoveRows();

//         remove_extra_objects(obj, model);
}

item_models::PropertyModelBase::Private::Subtree* item_models::PropertyModelBase::Private::visual_node_parent(Subtree* tree)
{
    while ( tree->parent )
    {
        tree = node(tree->parent);
        if ( !tree )
            return nullptr;

        if ( tree->visual_node )
            return tree;
    }

    return nullptr;
}

QModelIndex item_models::PropertyModelBase::Private::node_index(model::DocumentNode* node)
{
    auto it = objects.find(node);
    if ( it == objects.end() )
        return {};

    Subtree* tree = this->node(it->second);
    if ( !tree )
        return {};

    int row = 0;
    if ( Subtree* parent = visual_node_parent(tree) )
    {
        row = parent->visual_node->docnode_child_index(node);
        if ( row == -1 )
            return {};
    }
    else
    {
        for ( ; row < int(roots.size()); row++ )
            if ( roots[row]->visual_node == node )
                break;

        if ( row == int(roots.size()) )
            return {};
    }

    return model->createIndex(row, 0, tree->id);
}

QModelIndex item_models::PropertyModelBase::Private::subtree_index(id_type id)
{
    auto it = nodes.find(id);
    if ( it == nodes.end() )
        return {};
    return subtree_index(&it->second);
}

QModelIndex item_models::PropertyModelBase::Private::subtree_index(Subtree* tree)
{
    if ( !tree )
        return {};

    int row = 0;
    if ( tree->parent )
    {
        auto it = nodes.find(tree->parent);
        if ( it == nodes.end() )
            return {};

        auto parent = &*it;
        if ( !parent )
            return {};

        row = parent->second.child_index(tree);
        if ( row == -1 )
            return {};
    }
    else
    {
        for ( ; row < int(roots.size()); row++ )
            if ( roots[row] == tree )
                break;

        if ( row == int(roots.size()) )
            return {};
    }

    return model->createIndex(row, 0, tree->id);
}

bool item_models::PropertyModelBase::Private::set_prop_data(Subtree* tree, const QVariant& value, int role)
{
    if ( !tree->prop )
        return false;

    model::BaseProperty* prop = tree->prop;
    model::PropertyTraits traits = prop->traits();


    if ( (traits.flags & (model::PropertyTraits::List|model::PropertyTraits::ReadOnly)) ||
        traits.type == model::PropertyTraits::Object ||
        traits.type == model::PropertyTraits::Unknown )
    {
        return false;
    }
    else if ( traits.type == model::PropertyTraits::Bool )
    {
        if ( role == Qt::CheckStateRole )
        {
            return prop->set_undoable(value.value<Qt::CheckState>() == Qt::Checked);
        }
        return false;
    }
    else
    {
        if ( role == Qt::EditRole )
            return prop->set_undoable(value);
        return false;
    }
}



item_models::PropertyModelBase::PropertyModelBase(std::unique_ptr<Private> d)
    : d(std::move(d))
{
}

item_models::PropertyModelBase::~PropertyModelBase()
{
}

QModelIndex item_models::PropertyModelBase::index(int row, int column, const QModelIndex& parent) const
{
    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
    {
        if ( row >= 0 && row < int(d->roots.size()) )
            return createIndex(row, column, d->roots[row]->id);
        return {};
    }


    if ( row >= 0 && row < int(tree->children.size()) )
        return createIndex(row, column, tree->children[row]->id);

    return {};
}

QModelIndex item_models::PropertyModelBase::parent(const QModelIndex& child) const
{
    // tree_child is the subtree for the child index
    Private::Subtree* tree_child = d->node_from_index(child);

    // child is a root node, so parent is invalid
    if ( !tree_child || tree_child->parent == 0 )
        return {};

    // no parent? => error
    auto it = d->nodes.find(tree_child->parent);
    if ( it == d->nodes.end() )
        return {};

    // tree is the subtree we want to return
    Private::Subtree* tree = &it->second;

    // We have a root node
    if ( !tree->parent )
    {
        // failsafe
        if ( !d->document )
            return {};

        // main is 0
        int index = 0;
        // assets is 1
        if ( tree->object == d->document->assets() )
            index = 1;

        return createIndex(index, 0, tree->id);
    }

    // no parent? => error
    it = d->nodes.find(tree->parent);
    if ( it == d->nodes.end() )
        return {};

    // tree_parent is the parent of tree, grandparent of the model index
    Private::Subtree* tree_parent = &it->second;

    // We look for the row index for the parent index
    for ( int i = 0; i < int(tree_parent->children.size()); i++ )
        if ( tree_parent->children[i] == tree )
            return createIndex(i, 0, tree->id);

    return {};
}

void item_models::PropertyModelBase::set_document(model::Document* document)
{
    beginResetModel();
    d->document = document;
    d->clear();
    on_document_reset();
    endResetModel();
}

void item_models::PropertyModelBase::clear_document()
{
    set_document(nullptr);
}

item_models::PropertyModelBase::Item item_models::PropertyModelBase::item(const QModelIndex& index) const
{
    if ( Private::Subtree* st = d->node_from_index(index) )
    {
        Item item = st->object;
        if ( st->prop )
            item.property = st->prop;
        return item;
    }

    return {};
}


QModelIndex item_models::PropertyModelBase::property_index(model::BaseProperty* prop) const
{
    auto it = d->properties.find(prop);
    if ( it == d->properties.end() )
        return {};

    Private::Subtree* prop_node = d->node(it->second);
    if ( !prop_node )
        return {};

    Private::Subtree* parent = d->node(prop_node->parent);

    int i = std::find(parent->children.begin(), parent->children.end(), prop_node) - parent->children.begin();
    return createIndex(i, 1, prop_node->id);
}

QModelIndex item_models::PropertyModelBase::object_index(model::Object* obj) const
{
    auto it = d->objects.find(obj);
    if ( it == d->objects.end() )
        return {};

    Private::Subtree* prop_node = d->node(it->second);
    if ( !prop_node )
        return {};

    Private::Subtree* parent = d->node(prop_node->parent);

    int i = 0;
    if ( !parent )
        i = std::find(d->roots.begin(), d->roots.end(), prop_node) - d->roots.begin();
    else
        i = std::find(parent->children.begin(), parent->children.end(), prop_node) - parent->children.begin();

    return createIndex(i, 0, prop_node->id);
}

model::VisualNode* item_models::PropertyModelBase::visual_node(const QModelIndex& index) const
{
    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return nullptr;

    return tree->visual_node;
}

void item_models::PropertyModelBase::on_delete_object()
{
    d->on_delete_object(static_cast<model::Object*>(sender()));
}

void item_models::PropertyModelBase::property_changed(const model::BaseProperty* prop, const QVariant& value)
{
    d->property_changed(prop, value);
}

void item_models::PropertyModelBase::Private::property_changed(const model::BaseProperty* prop, const QVariant& value)
{
    auto it = properties.find(const_cast<model::BaseProperty*>(prop));
    if ( it != properties.end() )
        on_property_changed(it->second, prop, value);

    auto itref = referenced_properties.find(prop->object());
    if ( itref != referenced_properties.end() )
    {
        auto itprop = itref->second.find(const_cast<model::BaseProperty*>(prop));
        if ( itprop != itref->second.end() )
        {
            for ( id_type id : itprop->second )
                on_property_changed(id, prop, value);
        }
    }
}

void item_models::PropertyModelBase::Private::clean_object_references(const QModelIndex& index, Private::Subtree* prop_node)
{
    if ( prop_node->children.empty() )
        return;

    model->beginRemoveRows(index, 0, prop_node->children.size());

    prop_node->children.clear();

    auto itref = referenced_properties.find(prop_node->object);
    if ( itref != referenced_properties.end() )
    {
        auto itprop = itref->second.find(prop_node->prop);
        if ( itprop != itref->second.end() )
        {
            itprop->second.erase(
                std::remove_if(itprop->second.begin(), itprop->second.end(), [prop_node, this](id_type id){
                    auto n = node(id);
                    return !n || n->parent == prop_node->id;
                }),
                itprop->second.end()
            );
        }
    }

    model->endRemoveRows();
}

void item_models::PropertyModelBase::Private::on_property_changed(id_type prop_node_id, const model::BaseProperty*, const QVariant& value)
{
    Private::Subtree* prop_node = node(prop_node_id);
    if ( !prop_node )
        return;

    Private::Subtree* parent = node(prop_node->parent);

    if ( !parent )
        return;

    int i = std::find(parent->children.begin(), parent->children.end(), prop_node) - parent->children.begin();
    QModelIndex index = model->createIndex(i, 1, prop_node->id);

    if ( !(prop_node->prop->traits().flags & model::PropertyTraits::List) )
    {
        if ( prop_node->prop->traits().type == model::PropertyTraits::ObjectReference )
        {
            model::Object* obj_value = value.value<model::Object*>();

            if ( !prop_node->children.empty() )
                clean_object_references(index, prop_node);

            if ( prop_node->expand_referenced && obj_value )
            {
                connect_subobject(obj_value, prop_node, true);
            }

            prop_node->object = obj_value;
        }

        emit model->dataChanged(index, index, {});
    }
}


int item_models::PropertyModelBase::rowCount(const QModelIndex& parent) const
{
    if ( d->roots.empty() )
        return 0;

    Private::Subtree* tree = d->node_from_index(parent);
    if ( !tree )
        return d->roots.size();

    return tree->children.size();
}

QModelIndex item_models::PropertyModelBase::node_index(model::DocumentNode* node) const
{
    return object_index(node);
}

model::Document * item_models::PropertyModelBase::document() const
{
    return d->document;
}

model::DocumentNode * item_models::PropertyModelBase::node(const QModelIndex& index) const
{
    Private::Subtree* tree = d->node_from_index(index);
    if ( !tree )
        return nullptr;

    if ( tree->visual_node )
        return tree->visual_node;

    return qobject_cast<model::DocumentNode*>(tree->object);
}


item_models::PropertyModelBase::Private::Subtree*
item_models::PropertyModelBase::Private::add_property(
    model::BaseProperty* prop, id_type parent, bool, ReferencedPropertiesMap* referenced)
{
    Subtree* prop_node = add_node(Subtree{prop, parent});

    if ( referenced )
        (*referenced)[prop].push_back(prop_node->id);
    else
        properties[prop] = prop_node->id;

    return prop_node;
}
