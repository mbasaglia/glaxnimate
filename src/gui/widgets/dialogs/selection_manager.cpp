/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "selection_manager.hpp"

#include <QGuiApplication>
#include <QMimeData>
#include <QClipboard>

#include "model/assets/assets.hpp"
#include "model/shapes/precomp_layer.hpp"
#include "model/assets/pending_asset.hpp"

#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "glaxnimate_app.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;


model::ShapeElement* glaxnimate::gui::SelectionManager::current_shape() const
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_shape = qobject_cast<model::ShapeElement*>(curr) )
            return curr_shape;
    }
    return nullptr;
}

model::ShapeListProperty* glaxnimate::gui::SelectionManager::current_shape_container() const
{
    model::DocumentNode* sh = current_document_node();

    if ( !sh )
        return &current_composition()->shapes;

    if ( auto lay = qobject_cast<model::Composition*>(sh) )
        return &lay->shapes;

    if ( !qobject_cast<model::Layer*>(sh) )
        sh = sh->docnode_parent();

    while ( sh )
    {
        if ( auto grp = qobject_cast<model::Group*>(sh) )
            return &grp->shapes;
        if ( auto lay = qobject_cast<model::Composition*>(sh) )
            return &lay->shapes;
        sh = sh->docnode_parent();
    }

    return &current_composition()->shapes;
}

std::vector<model::VisualNode *> glaxnimate::gui::SelectionManager::copy() const
{
    auto selection = cleaned_selection();

    if ( !selection.empty() )
    {
        QMimeData* data = new QMimeData;
        for ( const auto& serializer : supported_mimes() )
        {
            serializer->to_mime_data(*data, std::vector<model::DocumentNode*>(selection.begin(), selection.end()));
        }

        GlaxnimateApp::instance()->set_clipboard_data(data);
    }

    return selection;
}

void glaxnimate::gui::SelectionManager::cut()
{
    auto selection = copy();

    delete_shapes_impl(QObject::tr("Cut"), selection);
}

void glaxnimate::gui::SelectionManager::delete_selected()
{
    delete_shapes_impl(QObject::tr("Delete"), cleaned_selection());
}

void glaxnimate::gui::SelectionManager::delete_shapes_impl(const QString &undo_string, const std::vector<model::VisualNode *>& selection)
{
    if ( selection.empty() )
        return;

    auto doc = document();
    command::UndoMacroGuard macro(undo_string, doc);
    auto current = this->current_document_node();

    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
        {
            if ( !shape->docnode_locked_recursive() )
            {
                if ( current->is_descendant_of(shape) )
                    current = shape->docnode_visual_parent();
                doc->push_command(new command::RemoveShape(shape, shape->owner()));
            }
        }
    }

    if ( current )
        set_current_document_node(current);
}

void glaxnimate::gui::SelectionManager::paste()
{
    paste_impl(false);
}

void glaxnimate::gui::SelectionManager::paste_as_composition()
{
    paste_impl(true);
}

template<class T>
static void paste_assets(model::SubObjectProperty<T> (model::Assets::* p), model::Document* source, model::Document* current_document)
{
    T* subject = (source->assets()->*p).get();
    T* target = (current_document->assets()->*p).get();

    for ( auto& item : subject->values.raw() )
    {
        if ( !current_document->assets()->find_by_uuid(item->uuid.get()) )
        {
            item->transfer(current_document);
            current_document->push_command(new command::AddObject(
                &target->values,
                std::move(item),
                target->values.size()
            ));
        }
    }
}

void glaxnimate::gui::SelectionManager::paste_impl(bool as_comp)
{
    const QMimeData* data = GlaxnimateApp::instance()->get_clipboard_data();

    io::mime::DeserializedData raw_pasted;
    for ( const auto& serializer : supported_mimes() )
    {
        raw_pasted = serializer->from_mime_data(*data);
        if ( !raw_pasted.empty() )
            break;
    }
    if ( raw_pasted.empty() )
    {
//        status_message(tr("Nothing to paste"));
        return;
    }

    paste_document(raw_pasted.document.get(), QObject::tr("Paste"), as_comp);
}

void glaxnimate::gui::SelectionManager::paste_document(model::Document* document, const QString& macro_name, bool as_comp)
{
    auto doc = this->document();

    auto& comps = document->assets()->compositions->values;

    command::UndoMacroGuard macro(macro_name, doc);
    paste_assets(&model::Assets::colors, document, doc);
    paste_assets(&model::Assets::images, document, doc);
    paste_assets(&model::Assets::gradient_colors, document, doc);
    paste_assets(&model::Assets::gradients, document, doc);

    model::ShapeListProperty* shape_cont = current_shape_container();
    std::vector<model::VisualNode*> select;

    if ( comps.empty() )
        return;
    if ( comps.size() > 1 )
        as_comp = true;

    model::Composition* comp = comps[0];

    if ( !as_comp )
    {
        if ( !comp->shapes.empty() )
        {
            int shape_insertion_point = shape_cont->size();
            for ( auto& shape : comp->shapes.raw() )
            {
                auto ptr = shape.get();
                shape->clear_owner();
                shape->refresh_uuid();
                if ( !as_comp )
                    select.push_back(ptr);
                shape->transfer(doc);
                doc->push_command(new command::AddShape(shape_cont, std::move(shape), shape_insertion_point++));
                if ( !as_comp )
                    ptr->recursive_rename();
            }
        }
    }
    else
    {
        paste_assets(&model::Assets::compositions, document, doc);
    }

    for ( const auto& pending : document->pending_assets() )
        doc->add_pending_asset(pending);

    if ( as_comp )
    {
        select.push_back(layer_new_comp(comp));
    }

    set_selection(select);
}

void glaxnimate::gui::SelectionManager::layer_new_impl(std::unique_ptr<model::ShapeElement> layer)
{
    auto doc = document();
    if ( layer->name.get().isEmpty() )
        doc->set_best_name(layer.get(), {});
    auto curr_dn = current_document_node();
    layer->set_time(curr_dn ? curr_dn->time() : doc->current_time());

    model::ShapeElement* ptr = layer.get();

    auto cont = current_shape_container();
    int position = cont->index_of(current_shape());
    if ( position >= 0 )
        position += 1;
    doc->push_command(new command::AddShape(cont, std::move(layer), position));

    set_current_document_node(ptr);
}


model::PreCompLayer* glaxnimate::gui::SelectionManager::layer_new_comp(model::Composition* comp)
{
    auto doc = document();
    auto layer = std::make_unique<model::PreCompLayer>(doc);
    layer->composition.set(comp);
    layer->name.set(comp->name.get());
    layer->size.set(comp->size());
    QPointF pos = current_composition()->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    auto ptr = layer.get();
    layer_new_impl(std::move(layer));
    return ptr;
}
