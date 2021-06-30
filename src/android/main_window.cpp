#include "main_window.hpp"
#include "ui_main_window.h"

#include <QPointer>
#include <QScreen>
#include "glaxnimate_app_android.hpp"

#include "model/document.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"

#include "graphics/document_scene.hpp"

#include "model/shapes/ellipse.hpp"
#include "tools/base.hpp"

class MainWindow::Private
{
public:
    MainWindow* parent;
    Ui::MainWindow ui;
    graphics::DocumentScene scene;
    std::unique_ptr<model::Document> current_document;
    tools::Tool* active_tool = nullptr;

    QPointer<model::BrushStyle> main_brush;
    QPointer<model::BrushStyle> secondary_brush;
    model::Composition* comp = nullptr;
    QPointer<model::VisualNode> current_node;

    void setup_document_new()
    {
        comp = nullptr;
        scene.set_document(nullptr);

        current_document = std::make_unique<model::Document>("");
        current_document->main()->width.set(512);
        current_document->main()->height.set(512);
        current_document->main()->animation->last_frame.set(180);
        current_document->main()->fps.set(60);


        auto layer = std::make_unique<model::Layer>(current_document.get());
        layer->animation->last_frame.set(180);
        layer->name.set(layer->type_name_human());
        QPointF pos(
            current_document->main()->width.get() / 2.0,
            current_document->main()->height.get() / 2.0
        );
        layer->transform.get()->anchor_point.set(pos);
        layer->transform.get()->position.set(pos);
        current_node = layer.get();

        current_document->main()->shapes.insert(std::move(layer), 0);

        scene.set_document(current_document.get());

        comp = current_document->main();
    }

    void switch_tool(tools::Tool* tool)
    {
        if ( !tool || tool == active_tool )
            return;

        if ( !tool->get_action()->isChecked() )
            tool->get_action()->setChecked(true);
/*
        if ( active_tool )
        {
            for ( const auto& widget : tool_widgets[active_tool->id()] )
            {
                widget->setVisible(false);
                widget->setEnabled(false);
            }

            for ( const auto& action : tool_actions[active_tool->id()] )
            {
                action->setEnabled(false);
            }
        }


        for ( const auto& widget : tool_widgets[tool->id()] )
        {
            widget->setVisible(true);
            widget->setEnabled(true);
        }

        for ( const auto& action : tool_actions[tool->id()] )
        {
            action->setEnabled(true);
        }
*/
        active_tool = tool;
        scene.set_active_tool(tool);
        ui.canvas->set_active_tool(tool);
        /*
        ui.tool_settings_widget->setCurrentWidget(tool->get_settings_widget());
        if ( active_tool->group() == tools::Registry::Draw || active_tool->group() == tools::Registry::Shape )
            widget_current_style->clear_gradients();*/
    }

    void init_tools_ui()
    {
        // Tool Actions
        QActionGroup *tool_actions = new QActionGroup(parent);
        tool_actions->setExclusive(true);

        for ( const auto& grp : tools::Registry::instance() )
        {
            for ( const auto& tool : grp.second )
            {
                QAction* action = tool.second->get_action();
                action->setParent(parent);
                ui.menu_tools->addAction(action);
                action->setActionGroup(tool_actions);
                connect(action, &QAction::triggered, parent, &MainWindow::tool_triggered);

                ui.toolbar_tools->addAction(action);

//                QWidget* widget = tool.second->get_settings_widget();
//                ui.tool_settings_widget->addWidget(widget);

                if ( !active_tool )
                {
                    active_tool = tool.second.get();
                    action->setChecked(true);
                }
            }
            ui.menu_tools->addSeparator();
            ui.toolbar_tools->addSeparator();
        }

        ui.menubar->setVisible(false);
        ui.statusbar->setVisible(false);
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    d(std::make_unique<Private>())
{
    d->parent = this;
    d->ui.setupUi(this);

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    d->init_tools_ui();
    d->ui.canvas->set_tool_target(this);
    d->ui.canvas->setScene(&d->scene);
    d->setup_document_new();

    connect(
        QGuiApplication::primaryScreen(),
        &QScreen::orientationChanged,
        this,
        &MainWindow::orientation_changed
    );
    orientation_changed(QGuiApplication::primaryScreen()->orientation());
}

MainWindow::~MainWindow()
{
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            d->ui.retranslateUi(this);
            break;
        default:
            break;
    }
}

model::Document* MainWindow::document() const
{
    return d->current_document.get();
}


model::Composition* MainWindow::current_composition()
{
    return d->comp;
}

model::VisualNode* MainWindow::current_document_node()
{
    return d->current_node;
}

QColor MainWindow::current_color() const
{
    return {};
}

void MainWindow::set_current_color(const QColor& c)
{

}

QColor MainWindow::secondary_color() const
{
    return {};
}

void MainWindow::set_secondary_color(const QColor& c)
{

}

QPen MainWindow::current_pen_style() const
{
    return {};
}

qreal MainWindow::current_zoom() const
{
    return d->ui.canvas->get_zoom_factor();
}

model::BrushStyle* MainWindow::linked_brush_style(bool secondary) const
{
    if ( secondary )
        return d->secondary_brush;
    return d->main_brush;
}

void MainWindow::set_current_document_node(model::VisualNode* node)
{
    d->current_node = node;
}

void MainWindow::set_current_composition(model::Composition* comp)
{
    d->comp = comp ? comp : d->current_document->main();
    d->scene.set_composition(comp);
}

void MainWindow::switch_tool(tools::Tool* tool)
{
    d->switch_tool(tool);
}

std::vector<model::VisualNode*> MainWindow::cleaned_selection() const
{
    return d->scene.cleaned_selection();
}


void MainWindow::tool_triggered(bool checked)
{
    if ( checked )
        d->switch_tool(static_cast<QAction*>(sender())->data().value<tools::Tool*>());
}

void MainWindow::orientation_changed(Qt::ScreenOrientation orientation)
{
    if ( QGuiApplication::primaryScreen()->isLandscape(orientation) )
    {
        d->ui.toolbar_tools->setAllowedAreas(Qt::LeftToolBarArea);
        d->ui.toolbar_tools->setOrientation(Qt::Horizontal);
    }
    else
    {
        d->ui.toolbar_tools->setOrientation(Qt::Vertical);
        d->ui.toolbar_tools->setAllowedAreas(Qt::BottomToolBarArea);
    }
}








































