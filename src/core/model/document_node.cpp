/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "document_node.hpp"
#include "document.hpp"

#include <QPainter>
#include <QGraphicsItem>

#include "model/shapes/shape.hpp"
#include "model/property/reference_property.hpp"
#include "utils/pseudo_mutex.hpp"

class glaxnimate::model::DocumentNode::Private
{
public:
    std::unordered_set<User*> users;
    utils::PseudoMutex detaching;
    DocumentNode* list_parent = nullptr;
};

glaxnimate::model::DocumentNode::DocumentNode(glaxnimate::model::Document* document)
    : DocumentNode(document, std::make_unique<Private>())
{
}

glaxnimate::model::DocumentNode::DocumentNode(glaxnimate::model::Document* document, std::unique_ptr<Private> d)
    : Object ( document ), d(std::move(d))
{
    uuid.set_value(QUuid::createUuid());
}


glaxnimate::model::DocumentNode::~DocumentNode() = default;

void glaxnimate::model::DocumentNode::removed_from_list()
{
    auto old = d->list_parent;
    d->list_parent = nullptr;
    document()->decrease_node_name(name.get());
    on_parent_changed(old, d->list_parent);
    emit removed();
}

void glaxnimate::model::DocumentNode::added_to_list ( glaxnimate::model::DocumentNode* new_parent )
{
    auto old = d->list_parent;
    d->list_parent = new_parent;
    document()->increase_node_name(name.get());
    on_parent_changed(old, d->list_parent);
}

void glaxnimate::model::DocumentNode::on_name_changed(const QString& name, const QString& old_name)
{
    if ( old_name != name )
    {
        document()->decrease_node_name(old_name);
        document()->increase_node_name(name);
        emit name_changed(name);
    }
}

glaxnimate::model::DocumentNode * glaxnimate::model::DocumentNode::docnode_parent() const
{
    return d->list_parent;
}

bool glaxnimate::model::DocumentNode::docnode_is_instance(const QString& type_name) const
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

void glaxnimate::model::DocumentNode::recursive_rename()
{
    document()->set_best_name(this, name.get());
    for ( auto child : docnode_children() )
        child->recursive_rename();
}


void glaxnimate::model::DocumentNode::refresh_uuid()
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
                    if ( auto obj = v.value<glaxnimate::model::DocumentNode*>() )
                        obj->refresh_uuid();
                }
            }
            else
            {
                if ( auto obj = qobject_cast<DocumentNode*>(static_cast<glaxnimate::model::SubObjectPropertyBase*>(prop)->sub_object()) )
                    obj->refresh_uuid();
            }
        }
    }
}

QString glaxnimate::model::DocumentNode::object_name() const
{
    if ( name.get().isEmpty() )
        return type_name_human();
    return name.get();
}

void glaxnimate::model::DocumentNode::add_user(glaxnimate::model::DocumentNode::User* user)
{
    if ( !d->detaching )
    {
        d->users.insert(user);
        emit users_changed();
    }
}

void glaxnimate::model::DocumentNode::remove_user(glaxnimate::model::DocumentNode::User* user)
{
    if ( !d->detaching )
    {
        d->users.erase(user);
        emit users_changed();
    }
}

const std::unordered_set<glaxnimate::model::DocumentNode::User*> & glaxnimate::model::DocumentNode::users() const
{
    return d->users;
}

void glaxnimate::model::DocumentNode::attach()
{
    if ( auto lock = d->detaching.get_lock() )
    {
        for ( auto user : d->users )
            user->set_ref(this);
    }
}

void glaxnimate::model::DocumentNode::detach()
{
    if ( auto lock = d->detaching.get_lock() )
    {
        for ( auto user : d->users )
            user->set_ref(nullptr);
    }
}

bool glaxnimate::model::DocumentNode::is_descendant_of(const model::DocumentNode* other) const
{
    if ( !other )
        return false;

    if ( other == this )
        return true;

    auto parent = docnode_parent();
    if ( parent )
        return parent->is_descendant_of(other);

    return false;
}


class glaxnimate::model::VisualNode::Private : public DocumentNode::Private
{
public:
    std::unique_ptr<QPixmap> group_icon;
};

glaxnimate::model::VisualNode::VisualNode(model::Document* document)
    : DocumentNode(document, std::make_unique<Private>())
{
}

glaxnimate::model::VisualNode::Private * glaxnimate::model::VisualNode::dd() const
{
    return static_cast<Private*>(d.get());
}

glaxnimate::model::VisualNode* glaxnimate::model::VisualNode::docnode_group_parent() const
{
    return nullptr;
}
int glaxnimate::model::VisualNode::docnode_group_child_count() const
{
    return 0;
}
glaxnimate::model::VisualNode* glaxnimate::model::VisualNode::docnode_group_child(int) const
{
    return nullptr;
}
glaxnimate::model::VisualNode* glaxnimate::model::VisualNode::docnode_fuzzy_parent() const
{
    if ( auto p = docnode_group_parent() )
        return p;
    return docnode_visual_parent();
}

QColor glaxnimate::model::VisualNode::docnode_group_color() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_fuzzy_parent() )
            return parent->docnode_group_color();

        return Qt::transparent;
    }
    return group_color.get();
}

glaxnimate::model::VisualNode* glaxnimate::model::VisualNode::docnode_visual_child(int index) const
{
    return static_cast<glaxnimate::model::VisualNode*>(docnode_child(index));
}

glaxnimate::model::VisualNode* glaxnimate::model::VisualNode::docnode_visual_parent() const
{
    auto p = docnode_parent();
    if ( p )
        return p->cast<glaxnimate::model::VisualNode>();
    return nullptr;
}

void glaxnimate::model::VisualNode::on_group_color_changed(const QColor&)
{
    if ( dd()->group_icon && !dd()->group_icon->isNull() )
    {
        if ( docnode_valid_color() )
            dd()->group_icon->fill(group_color.get());
        else
            dd()->group_icon->fill(Qt::white);
    }
    docnode_on_update_group(true);
}


void glaxnimate::model::VisualNode::docnode_on_update_group(bool)
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

bool glaxnimate::model::VisualNode::docnode_valid_color() const
{
    QColor col = group_color.get();
    return col.isValid() && col.alpha() > 0;
}

QIcon glaxnimate::model::VisualNode::instance_icon() const
{
    if ( !docnode_valid_color() )
    {
        if ( auto parent = docnode_fuzzy_parent() )
            return parent->instance_icon();
    }

    if ( !dd()->group_icon )
    {
        dd()->group_icon = std::make_unique<QPixmap>(33, 33);
        dd()->group_icon->fill(docnode_group_color());
    }

    return *dd()->group_icon;
}

bool glaxnimate::model::VisualNode::docnode_locked_recursive() const
{
    for ( const VisualNode* n = this; n; n = n->docnode_visual_parent() )
    {
        if ( n->locked.get() )
            return true;
    }

    return false;
}

void glaxnimate::model::VisualNode::paint(QPainter* painter, FrameTime time, PaintMode mode, glaxnimate::model::Modifier* modifier) const
{
    if ( !visible.get() )
        return;

    painter->save();
    painter->setTransform(group_transform_matrix(time), true);

    on_paint(painter, time, mode, modifier);
    for ( auto c : docnode_visual_children() )
    {
        c->paint(painter, time, mode, modifier);
        if ( c->is_instance<glaxnimate::model::Modifier>() )
            break;
    }
    painter->restore();
}

bool glaxnimate::model::VisualNode::docnode_selectable() const
{
    if ( !visible.get() || locked.get() )
        return false;
    if ( auto p = docnode_visual_parent() )
        return p->docnode_selectable();
    return true;
}

bool glaxnimate::model::VisualNode::docnode_visible_recursive() const
{
    if ( !visible.get() )
        return false;
    if ( auto p = docnode_visual_parent() )
        return p->docnode_visible_recursive();
    return true;
}

QTransform glaxnimate::model::VisualNode::transform_matrix(glaxnimate::model::FrameTime t) const
{
    auto matrix = local_transform_matrix(t);

    glaxnimate::model::VisualNode* parent = docnode_visual_parent();
    if ( parent )
        matrix *= parent->transform_matrix(t);

    parent = docnode_group_parent();
    if ( parent )
        matrix *= parent->transform_matrix(t);

    return matrix;
}

QTransform glaxnimate::model::VisualNode::group_transform_matrix(glaxnimate::model::FrameTime t) const
{
    auto parent = docnode_group_parent();
    if ( parent )
        return local_transform_matrix(t) * parent->transform_matrix(t);
    return local_transform_matrix(t);
}

void glaxnimate::model::VisualNode::on_visible_changed(bool visible)
{
    emit docnode_visible_changed(visible);
    emit docnode_visible_recursive_changed(visible);

    for ( auto ch : docnode_visual_children() )
        ch->propagate_visible(visible);
}

void glaxnimate::model::VisualNode::propagate_visible(bool visible)
{
    if ( !this->visible.get() )
        return;
    emit docnode_visible_recursive_changed(visible);
    for ( auto ch : docnode_visual_children() )
        ch->propagate_visible(visible && this->visible.get());
}

void glaxnimate::model::VisualNode::propagate_transform_matrix_changed(const QTransform& t_global, const QTransform& t_group)
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

void glaxnimate::model::VisualNode::propagate_bounding_rect_changed()
{
    on_graphics_changed();
    emit bounding_rect_changed();
    if ( auto parent = docnode_visual_parent() )
        parent->propagate_bounding_rect_changed();

}
