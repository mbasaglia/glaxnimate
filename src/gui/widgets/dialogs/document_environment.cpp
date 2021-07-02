#include "document_environment.hpp"

#include <QGuiApplication>
#include <QMimeData>
#include <QClipboard>

#include "model/assets/assets.hpp"
#include "model/shapes/precomp_layer.hpp"

#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "command/undo_macro_guard.hpp"


model::ShapeElement* glaxnimate::gui::DocumentEnvironment::current_shape() const
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_shape = qobject_cast<model::ShapeElement*>(curr) )
            return curr_shape;
    }
    return nullptr;
}

model::ShapeListProperty* glaxnimate::gui::DocumentEnvironment::current_shape_container() const
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

void glaxnimate::gui::DocumentEnvironment::delete_selected()
{
    auto selection = cleaned_selection();
    if ( selection.empty() )
        return;

    auto doc = document();

    command::UndoMacroGuard macro(QObject::tr("Delete"), doc);
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
            if ( !shape->locked.get() )
                doc->push_command(new command::RemoveShape(shape, shape->owner()));
    }
}

#include <QDebug>
std::vector<model::VisualNode *> glaxnimate::gui::DocumentEnvironment::copy() const
{
    auto selection = cleaned_selection();

    if ( !selection.empty() )
    {
        QMimeData* data = new QMimeData;
        for ( const auto& serializer : supported_mimes() )
        {
            serializer->to_mime_data(*data, std::vector<model::DocumentNode*>(selection.begin(), selection.end()));
        }

        QGuiApplication::clipboard()->setMimeData(data);
    }

    return selection;
}

void glaxnimate::gui::DocumentEnvironment::cut()
{
    auto selection = copy();
    if ( selection.empty() )
        return;

    auto doc = document();
    command::UndoMacroGuard macro(QObject::tr("Cut"), doc);
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(item) )
            if ( !shape->locked.get() )
                doc->push_command(new command::RemoveShape(shape, shape->owner()));
    }
}

void glaxnimate::gui::DocumentEnvironment::paste()
{
    paste_impl(false);
}

void glaxnimate::gui::DocumentEnvironment::paste_as_composition()
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

void glaxnimate::gui::DocumentEnvironment::paste_impl(bool as_comp)
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();

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


void glaxnimate::gui::DocumentEnvironment::paste_document(model::Document* document, const QString& macro_name, bool as_comp)
{
    auto doc = this->document();
    command::UndoMacroGuard macro(macro_name, doc);
    paste_assets(&model::Assets::colors, document, doc);
    paste_assets(&model::Assets::images, document, doc);
    paste_assets(&model::Assets::gradient_colors, document, doc);
    paste_assets(&model::Assets::gradients, document, doc);
    paste_assets(&model::Assets::precompositions, document, doc);

    model::ShapeListProperty* shape_cont = current_shape_container();
    std::vector<model::VisualNode*> select;

    if ( as_comp )
    {
        std::unique_ptr<model::Precomposition> comp = std::make_unique<model::Precomposition>(doc);
        auto comp_ptr = comp.get();
        doc->set_best_name(comp.get(), document->main()->name.get());
        doc->push_command(new command::AddObject(&doc->assets()->precompositions->values, std::move(comp)));

        select.push_back(layer_new_comp(comp_ptr));
        shape_cont = &comp_ptr->shapes;
    }

    if ( !document->main()->shapes.empty() )
    {
        int shape_insertion_point = shape_cont->size();
        for ( auto& shape : document->main()->shapes.raw() )
        {
            auto ptr = shape.get();
            shape->refresh_uuid();
            if ( !as_comp )
                select.push_back(ptr);
            shape->transfer(doc);
            doc->push_command(new command::AddShape(shape_cont, std::move(shape), shape_insertion_point++));
            ptr->recursive_rename();
        }
    }

    set_selection(select);
}

void glaxnimate::gui::DocumentEnvironment::layer_new_impl(std::unique_ptr<model::ShapeElement> layer)
{
    auto doc = document();
    doc->set_best_name(layer.get(), {});
    layer->set_time(current_document_node()->time());

    model::ShapeElement* ptr = layer.get();

    auto cont = current_shape_container();
    int position = cont->index_of(current_shape());
    doc->push_command(new command::AddShape(cont, std::move(layer), position));

    set_current_document_node(ptr);
}


model::PreCompLayer* glaxnimate::gui::DocumentEnvironment::layer_new_comp(model::Precomposition* comp)
{
    auto doc = document();
    auto layer = std::make_unique<model::PreCompLayer>(doc);
    layer->composition.set(comp);
    layer->size.set(doc->rect().size());
    QPointF pos = doc->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    auto ptr = layer.get();
    layer_new_impl(std::move(layer));
    return ptr;
}
