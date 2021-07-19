#include "document_node.hpp"
#include "document.hpp"

#include <QPainter>
#include <QGraphicsItem>

#include "model/shapes/shape.hpp"
#include "model/property/reference_property.hpp"
#include "utils/pseudo_mutex.hpp"

class model::DocumentNode::Private
{
public:
    std::unordered_set<User*> users;
    utils::PseudoMutex detaching;
};

model::DocumentNode::DocumentNode(model::Document* document)
    : Object ( document ), d(std::make_unique<Private>())
{
    uuid.set_value(QUuid::createUuid());
}

model::DocumentNode::~DocumentNode() = default;

model::VisualNode* model::VisualNode::docnode_group_parent() const
{
    return nullptr;
}
int model::VisualNode::docnode_group_child_count() const
{
    return 0;
}
model::VisualNode* model::VisualNode::docnode_group_child(int) const
{
    return nullptr;
}
model::VisualNode* model::VisualNode::docnode_fuzzy_parent() const
{
    if ( auto p = docnode_group_parent() )
        return p;
    return docnode_visual_parent();
}

QColor model::VisualNode::docnode_group_color() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_fuzzy_parent() )
            return parent->docnode_group_color();

        return Qt::transparent;
    }
    return group_color.get();
}

model::VisualNode* model::VisualNode::docnode_visual_child(int index) const
{
    return static_cast<model::VisualNode*>(docnode_child(index));
}

model::VisualNode* model::VisualNode::docnode_visual_parent() const
{
    auto p = docnode_parent();
    if ( p )
        return p->cast<model::VisualNode>();
    return nullptr;
}

void model::VisualNode::on_group_color_changed(const QColor&)
{
    if ( !group_icon.isNull() )
    {
        if ( docnode_valid_color() )
            group_icon.fill(group_color.get());
        else
            group_icon.fill(Qt::white);
    }
    docnode_on_update_group(true);
}

bool model::DocumentNode::docnode_is_instance(const QString& type_name) const
{
    if ( type_name.isEmpty() )
        return true;

    for ( const QMetaObject* meta = metaObject(); meta; meta = meta->superClass() )
    {
        if ( detail::naked_type_name(meta->className()) == type_name )
            return true;
    }

    return false;
}

void model::VisualNode::docnode_on_update_group(bool)
{
//     if ( force || docnode_valid_color() )
    {
        emit docnode_group_color_changed(docnode_group_color());
        for ( auto gc : docnode_group_children() )
            gc->docnode_on_update_group();
        for ( auto gc : docnode_visual_children() )
            gc->docnode_on_update_group();
    }

    emit group_transform_matrix_changed(group_transform_matrix(time()));
}

bool model::VisualNode::docnode_valid_color() const
{
    QColor col = group_color.get();
    return col.isValid() && col.alpha() > 0;
}

QIcon model::VisualNode::instance_icon() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_fuzzy_parent() )
            return parent->instance_icon();
    }

    if ( group_icon.isNull() )
    {
        group_icon = QPixmap{32, 32};
        group_icon.fill(docnode_group_color());
    }

    return group_icon;
}

bool model::VisualNode::docnode_locked_recursive() const
{
    for ( const VisualNode* n = this; n; n = n->docnode_visual_parent() )
    {
        if ( n->locked.get() )
            return true;
    }

    return false;
}

void model::VisualNode::paint(QPainter* painter, FrameTime time, PaintMode mode, model::Modifier* modifier) const
{
    if ( !visible.get() )
        return;

    painter->save();
    painter->setTransform(group_transform_matrix(time), true);

    on_paint(painter, time, mode, modifier);
    for ( auto c : docnode_visual_children() )
    {
        c->paint(painter, time, mode, modifier);
        if ( c->is_instance<model::Modifier>() )
            break;
    }
    painter->restore();
}

bool model::VisualNode::docnode_selectable() const
{
    if ( !visible.get() || locked.get() )
        return false;
    if ( auto p = docnode_visual_parent() )
        return p->docnode_selectable();
    return true;
}

bool model::VisualNode::docnode_visible_recursive() const
{
    if ( !visible.get() )
        return false;
    if ( auto p = docnode_visual_parent() )
        return p->docnode_visible_recursive();
    return true;
}

QTransform model::VisualNode::transform_matrix(model::FrameTime t) const
{
    auto matrix = local_transform_matrix(t);

    model::VisualNode* parent = docnode_visual_parent();
    if ( parent )
        matrix *= parent->transform_matrix(t);

    parent = docnode_group_parent();
    if ( parent )
        matrix *= parent->transform_matrix(t);

    return matrix;
}

QTransform model::VisualNode::group_transform_matrix(model::FrameTime t) const
{
    auto parent = docnode_group_parent();
    if ( parent )
        return local_transform_matrix(t) * parent->transform_matrix(t);
    return local_transform_matrix(t);
}

/// \todo This is very inefficient, it should cache the list of available names
void model::DocumentNode::recursive_rename()
{
    document()->set_best_name(this, name.get());
    for ( auto child : docnode_children() )
        child->recursive_rename();
}

void model::VisualNode::on_visible_changed(bool visible)
{
    emit docnode_visible_changed(visible);
    emit docnode_visible_recursive_changed(visible);

    for ( auto ch : docnode_visual_children() )
        ch->propagate_visible(visible);
}

void model::VisualNode::propagate_visible(bool visible)
{
    if ( !this->visible.get() )
        return;
    emit docnode_visible_recursive_changed(visible);
    for ( auto ch : docnode_visual_children() )
        ch->propagate_visible(visible && this->visible.get());
}

void model::VisualNode::propagate_transform_matrix_changed(const QTransform& t_global, const QTransform& t_group)
{
    emit transform_matrix_changed(t_global);
    emit group_transform_matrix_changed(t_group);

    for ( auto ch : docnode_group_children() )
    {
        auto ltm = ch->local_transform_matrix(ch->time());
        ch->propagate_transform_matrix_changed(ltm * t_global, ltm * t_group);
    }

    for ( auto ch : docnode_visual_children() )
    {
        auto ltm = ch->local_transform_matrix(ch->time());
        ch->propagate_transform_matrix_changed(ltm * t_global, ltm);
    }
}

void model::DocumentNode::refresh_uuid()
{
    uuid.set_value(QUuid::createUuid());
    for ( auto prop : properties() )
    {
        if ( prop->traits().type == PropertyTraits::Object )
        {
            if ( prop->traits().flags & PropertyTraits::List )
            {
                for ( auto v : prop->value().toList() )
                {
                    if ( auto obj = v.value<model::DocumentNode*>() )
                        obj->refresh_uuid();
                }
            }
            else
            {
                if ( auto obj = qobject_cast<DocumentNode*>(static_cast<model::SubObjectPropertyBase*>(prop)->sub_object()) )
                    obj->refresh_uuid();
            }
        }
    }
}

QString model::DocumentNode::object_name() const
{
    if ( name.get().isEmpty() )
        return type_name_human();
    return name.get();
}

void model::DocumentNode::add_user(model::DocumentNode::User* user)
{
    if ( !d->detaching )
    {
        d->users.insert(user);
        emit users_changed();
    }
}

void model::DocumentNode::remove_user(model::DocumentNode::User* user)
{
    if ( !d->detaching )
    {
        d->users.erase(user);
        emit users_changed();
    }
}

const std::unordered_set<model::DocumentNode::User*> & model::DocumentNode::users() const
{
    return d->users;
}

void model::DocumentNode::attach()
{
    if ( auto lock = d->detaching.get_lock() )
    {
        for ( auto user : d->users )
            user->set_ref(this);
    }
}

void model::DocumentNode::detach()
{
    if ( auto lock = d->detaching.get_lock() )
    {
        for ( auto user : d->users )
            user->set_ref(nullptr);
    }
}

