#ifndef GLAXNIMATEWINDOW_P_H
#define GLAXNIMATEWINDOW_P_H

#include <QGraphicsView>

#include "ui_glaxnimate_window.h"

#include "model/document.hpp"


class GlaxnimateWindow::Private
{
public:
    Ui::GlaxnimateWindow ui;

    int tool_rows = 3;
    std::vector<std::unique_ptr<model::Document>> documents;
    QString undo_text;
    QString redo_text;


    model::Document* current_document()
    {
        int index = ui.tab_widget->currentIndex();
        if ( index == -1 )
            return nullptr;

        return documents[index].get();
    }

    model::Document* create_document(const QString& filename)
    {
        documents.push_back(std::make_unique<model::Document>(filename));
        auto widget = new QGraphicsView();
        ui.tab_widget->addTab(widget, filename);
        model::Document* doc = documents.back().get();
        QObject::connect(doc, &model::Document::filename_changed, widget, &QWidget::setWindowTitle);
        switch_to_document(doc);
        return doc;
    }

    void switch_to_document(model::Document* document)
    {
        // Undo Redo
        QObject::connect(&document->undo_stack(), &QUndoStack::canRedoChanged, ui.action_redo, &QAction::setEnabled);
        QObject::connect(&document->undo_stack(), &QUndoStack::redoTextChanged, ui.action_redo, [this](const QString& s){
            ui.action_redo->setText(redo_text.arg(s));
        });
        ui.action_redo->setEnabled(document->undo_stack().canRedo());
        ui.action_redo->setText(redo_text.arg(document->undo_stack().redoText()));

        QObject::connect(&document->undo_stack(), &QUndoStack::canUndoChanged, ui.action_undo, &QAction::setEnabled);
        QObject::connect(&document->undo_stack(), &QUndoStack::undoTextChanged, ui.action_undo, [this](const QString& s){
            ui.action_undo->setText(undo_text.arg(s));
        });
        ui.action_undo->setEnabled(document->undo_stack().canUndo());
        ui.action_undo->setText(redo_text.arg(document->undo_stack().undoText()));
    }


    void setupUi(QMainWindow* parent)
    {
        ui.setupUi(parent);

        // Standard Shorcuts
        ui.action_new->setShortcut(QKeySequence::New);
        ui.action_open->setShortcut(QKeySequence::Open);
        ui.action_close->setShortcut(QKeySequence::Close);
        ui.action_reload->setShortcut(QKeySequence::Refresh);
        ui.action_save->setShortcut(QKeySequence::Save);
        ui.action_save_as->setShortcut(QKeySequence::SaveAs);
        ui.action_quit->setShortcut(QKeySequence::Quit);
        ui.action_copy->setShortcut(QKeySequence::Copy);
        ui.action_cut->setShortcut(QKeySequence::Cut);
        ui.action_paste->setShortcut(QKeySequence::Paste);
        ui.action_select_all->setShortcut(QKeySequence::SelectAll);
        ui.action_undo->setShortcut(QKeySequence::Undo);
        ui.action_redo->setShortcut(QKeySequence::Redo);

        // Menu Views
        for ( QDockWidget* wid : parent->findChildren<QDockWidget*>() )
        {
            ui.menu_views->addAction(wid->toggleViewAction());
        }

        // Tool Actions
        QActionGroup *tool_actions = new QActionGroup(parent);
        tool_actions->setExclusive(true);

        int row = 0;
        int column = 0;
        for ( QAction* action : ui.menu_tools->actions() )
        {
            if ( action->isSeparator() )
                continue;

            action->setActionGroup(tool_actions);

            QToolButton *button = new QToolButton(ui.dock_tools_contents);

            button->setIcon(action->icon());
            button->setCheckable(true);
            button->setChecked(action->isChecked());

            update_tool_button(action, button);
            QObject::connect(action, &QAction::changed, button, [action, button, this](){
                update_tool_button(action, button);
            });
            QObject::connect(button, &QToolButton::toggled, action, &QAction::setChecked);
            QObject::connect(action, &QAction::toggled, button, &QToolButton::setChecked);

            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            button->installEventFilter(parent);

            ui.dock_tools_layout->addWidget(button, row, column);

            column++;
            if ( column >= tool_rows )
            {
                column = 0;
                row++;
            }
        }
    }

    void retranslateUi(QMainWindow* parent)
    {
        ui.retranslateUi(parent);
        redo_text = ui.action_redo->text();
        undo_text = ui.action_undo->text();
    }

    void update_tool_button(QAction* action, QToolButton* button)
    {
        button->setText(action->text());
        button->setToolTip(action->text());
    }

    bool eventFilter(QObject* object, QEvent* event)
    {
        QToolButton *btn = qobject_cast<QToolButton*>(object);
        if ( btn && event->type() == QEvent::Resize )
        {
            int target = btn->size().width() - 10;
            QSize best(0, 0);
            for ( const auto& sz : btn->icon().availableSizes() )
            {
                if ( sz.width() > best.width() && sz.width() <= target )
                    best = sz;
            }
            if ( best.width() > 0 )
                btn->setIconSize(best);
        }

        return false;
    }

};

#endif // GLAXNIMATEWINDOW_P_H
