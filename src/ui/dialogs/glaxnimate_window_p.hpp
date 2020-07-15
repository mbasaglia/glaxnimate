#ifndef GLAXNIMATEWINDOW_P_H
#define GLAXNIMATEWINDOW_P_H

#include "ui_glaxnimate_window.h"
#include <QUndoStack>

#include "model/document.hpp"


class GlaxnimateWindow::Private
{
public:
    Ui::GlaxnimateWindow ui;

    int tool_rows = 3;
    QUndoStack undo_stack;
    QList<model::Document> documents;


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

        // undo - redo
        // TODO move undo_stack into the document, create actions in the UI
        //      and connect signals to update text/enabled
        QAction *redo = undo_stack.createRedoAction(parent);
        redo->setIcon(QIcon::fromTheme("edit-redo"));
        redo->setShortcut(QKeySequence::Undo);
        QAction *undo = undo_stack.createUndoAction(parent);
        undo->setIcon(QIcon::fromTheme("edit-undo"));
        undo->setShortcut(QKeySequence::Redo);
        ui.menu_edit->insertAction(ui.menu_edit->actions()[0], redo);
        ui.menu_edit->insertAction(redo, undo);
        QAction *separator_undo = ui.toolbar_main->insertSeparator(ui.action_copy);
        ui.toolbar_main->insertAction(separator_undo, redo);
        ui.toolbar_main->insertAction(redo, undo);

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
    }

    void update_tool_button(QAction* action, QToolButton* button)
    {
        button->setText(action->text());
        button->setToolTip(action->text()+"|"+action->iconText());
    }
};

#endif // GLAXNIMATEWINDOW_P_H
