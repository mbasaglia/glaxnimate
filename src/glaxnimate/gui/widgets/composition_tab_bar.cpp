#include "composition_tab_bar.hpp"

#include <QMenu>
#include <QIcon>
#include <QSignalBlocker>
#include <QInputDialog>

#include "glaxnimate/core/model/assets/assets.hpp"
#include "glaxnimate/core/command/object_list_commands.hpp"

#include "widgets/tab_bar_close_button.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;


CompositionTabBar::CompositionTabBar(QWidget* parent)
    : ClickableTabBar(parent)
{
    setDocumentMode(true);
    setExpanding(false);
    setAutoHide(true);
    setDrawBase(false);

    connect(this, &QTabBar::currentChanged, this, &CompositionTabBar::fw_switch);
    connect(this, &QTabBar::tabCloseRequested, this, &CompositionTabBar::on_close);
    connect(this, &ClickableTabBar::context_menu_requested, this, &CompositionTabBar::on_menu);
}

model::Composition * CompositionTabBar::index_to_comp(int index) const
{
    if ( !document || index < 0 )
        return nullptr;

    if ( index == 0 )
        return document->main();

    if ( index - 1 >= document->assets()->precompositions->values.size() )
        return nullptr;

    return document->assets()->precompositions->values[index - 1];
}


model::Precomposition * CompositionTabBar::index_to_precomp(int index) const
{
    index--;

    if ( !document || index < 0 || index >= document->assets()->precompositions->values.size() )
        return nullptr;

    return document->assets()->precompositions->values[index];
}

void CompositionTabBar::fw_switch(int i)
{
    if ( auto comp = index_to_comp(i) )
        emit switch_composition(comp, i);
}

void CompositionTabBar::on_close(int index)
{
    if ( document && index > 0 && index <= document->assets()->precompositions->values.size() )
    {
        document->push_command(new command::RemoveObject<model::Precomposition>(
            index-1, &document->assets()->precompositions->values
        ));
    }
}

void CompositionTabBar::on_menu(int index)
{
    if ( auto precomp = index_to_precomp(index) )
    {
        QMenu menu;
        menu.addSection(precomp->object_name());
        menu.addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), precomp, [this, index]{
            on_close(index);
        });
        menu.addAction(QIcon::fromTheme("edit-duplicate"), tr("Duplicate"), precomp, [this, precomp]{
            std::unique_ptr<model::Precomposition> new_comp (
                static_cast<model::Precomposition*>(precomp->clone().release())
            );
            new_comp->recursive_rename();
            new_comp->set_time(document->current_time());

            document->push_command(
                new command::AddObject(
                    &document->assets()->precompositions->values,
                    std::move(new_comp),
                    -1,
                    nullptr,
                    QObject::tr("Duplicate %1").arg(precomp->object_name())
                )
            );
        });
        menu.addAction(QIcon::fromTheme("edit-rename"), tr("Rename"), precomp, [this, precomp]{
            bool ok = false;
            QString str = QInputDialog::getText(
                this, tr("Rename Composition"), tr("Name"), QLineEdit::Normal,
                precomp->object_name(), &ok
            );
            if ( ok )
                precomp->name.set_undoable(str);
        });

        menu.exec(QCursor::pos());
    }
}

void CompositionTabBar::set_document(model::Document* document)
{
    QSignalBlocker guard(this);

    if ( this->document )
    {
        disconnect(this->document->assets()->precompositions.get(), nullptr, this, nullptr);
    }

    while ( count() )
        removeTab(0);

    this->document = document;

    if ( document )
    {
        setup_composition(document->main(), 0);
        for ( int i = 0; i < document->assets()->precompositions->values.size(); i++ )
            setup_composition(document->assets()->precompositions->values[i], i+1);

        connect(document->assets()->precompositions.get(), &model::PrecompositionList::docnode_child_remove_begin, this, &CompositionTabBar::on_precomp_removed);
        connect(document->assets()->precompositions.get(), &model::PrecompositionList::precomp_added, this, [this](model::Precomposition* node, int row){setup_composition(node, row+1);});
    }
}

void CompositionTabBar::setup_composition(model::Composition* comp, int index)
{
    index = insertTab(index, comp->tree_icon(), comp->object_name());
    update_comp_color(index, comp);

    if ( comp != document->main() )
        TabBarCloseButton::add_button(this, index);

    connect(comp, &model::DocumentNode::name_changed, this, [this, index, comp](){
        setTabText(index, comp->object_name());
    });
    connect(comp, &model::VisualNode::docnode_group_color_changed, this, [this, index, comp](){
        update_comp_color(index, comp);
    });
}

void CompositionTabBar::update_comp_color(int index, model::Composition* comp)
{
    QColor c = comp->group_color.get();
    if ( c.alpha() == 0 )
        c = palette().text().color();
    else
        c.setAlpha(255);

    setTabTextColor(index, c);
}

void CompositionTabBar::set_current_composition(model::Composition* comp)
{
    int index = document->assets()->precompositions->values.index_of(static_cast<model::Precomposition*>(comp));
    setCurrentIndex(index+1);
}

void CompositionTabBar::on_precomp_removed(int index)
{
    index++;
    if ( currentIndex() == index )
        setCurrentIndex(0);

    removeTab(index);
}
