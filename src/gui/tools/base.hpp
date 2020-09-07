#pragma once

#include <map>
#include <memory>

#include <QMouseEvent>
#include <QKeyEvent>
#include <QAction>

#include "graphics/document_scene.hpp"
#include "graphics/document_node_graphics_item.hpp"
#include "widgets/glaxnimate_graphics_view.hpp"
#include "widgets/scalable_button.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"
#include "app/settings/setting.hpp"
#include "app/settings/widget_builder.hpp"


namespace tools {

struct Event
{
    GlaxnimateGraphicsView* view;
    graphics::DocumentScene* scene;
    GlaxnimateWindow* window;

    void repaint() const
    {
        view->viewport()->update();
    }
};

struct MouseEvent : Event
{
    QMouseEvent* event;
    QPointF scene_pos;

    Qt::MouseButton press_button;
    QPointF press_scene_pos;
    QPoint press_screen_pos;
    QPointF last_scene_pos;
    QPoint last_screen_pos;

    Qt::KeyboardModifiers modifiers() const { return event->modifiers(); }
    Qt::MouseButton button() const { return event->button(); }
    Qt::MouseButtons buttons() const { return event->buttons(); }
    const QPointF& pos() const { return event->localPos(); }
    void accept() const { event->accept(); }

    void forward_to_scene() const;
};

struct PaintEvent : Event
{
    QPainter* painter;
    QPalette palette;
};

struct KeyEvent : public Event
{
    QKeyEvent* event;

    Qt::KeyboardModifiers modifiers() const { return event->modifiers(); }
    int key() const { return event->key(); }
    QString text() const { return event->text(); }
    void accept() const { event->accept(); }
};


using Priority = int;

class Tool
{
    Q_GADGET

public:
    virtual ~Tool() = default;

    virtual QIcon icon() const = 0;
    virtual QString name() const = 0;
    virtual QString tooltip() const { return name(); }
    virtual QKeySequence key_sequence() const = 0;
    virtual app::settings::SettingList settings() const = 0;

    QAction* get_action()
    {
        if ( !action )
        {
            action = new QAction();
            action->setCheckable(true);
            action->setIcon(icon());
            action->setData(QVariant::fromValue(this));
        }
        return action;
    }

    /**
     * \pre get_action() called before calling this
     */
    ScalableButton* get_button()
    {
        if ( !button )
        {
            button = new ScalableButton();

            button->setIcon(icon());
            button->setCheckable(true);
            button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

            QObject::connect(action, &QAction::toggled, button, &QAbstractButton::setChecked);
            QObject::connect(button, &QAbstractButton::clicked, action, &QAction::trigger);
        }

        return button;
    }

    QWidget* get_settings_widget()
    {
        if ( !settings_widget )
        {
            settings_widget = on_create_widget();
        }

        return settings_widget;
    }

    /**
     * \pre get_action and get_button already called
     */
    void retranslate()
    {
        action->setText(name());
        action->setToolTip(tooltip());
        action->setShortcut(key_sequence());

        button->setText(name());
        button->setToolTip(QObject::tr("%1 (%2)").arg(name()).arg(key_sequence().toString()));

        app::settings::WidgetBuilder().translate_widgets(settings(), settings_widget);
        on_translate();
    }

    virtual void mouse_press(const MouseEvent& event) = 0;
    virtual void mouse_move(const MouseEvent& event) = 0;
    virtual void mouse_release(const MouseEvent& event) = 0;
    virtual void mouse_double_click(const MouseEvent& event) = 0;
    virtual void paint(const PaintEvent& event) = 0;
    virtual void key_press(const KeyEvent& event) = 0;
    virtual void key_release(const KeyEvent& event) = 0;
    virtual QCursor cursor() = 0;
    virtual bool show_editors(model::DocumentNode* node) const = 0;
    virtual void enable_event(const Event& event) = 0;
    virtual void disable_event(const Event& event) = 0;

protected:
    bool mouse_on_handle(const MouseEvent& event) const
    {
        for ( auto item : event.scene->items(event.scene_pos, Qt::IntersectsItemShape, Qt::DescendingOrder, event.view->viewportTransform()) )
            if ( item->flags() & QGraphicsItem::ItemIsFocusable && !event.scene->item_to_node(item) )
                return true;
        return false;
    }

    virtual QWidget* on_create_widget()
    {
        QWidget* widget = new QWidget();
        QFormLayout* layout = new QFormLayout(widget);
        app::settings::WidgetBuilder().add_widgets(settings(), widget, layout, settings_values);
        return widget;
    }

    virtual void on_translate() {}


    QVariantMap settings_values;

    static constexpr Priority max_priority = std::numeric_limits<Priority>::min();

private:
    QAction* action = nullptr;
    ScalableButton* button = nullptr;
    QWidget* settings_widget = nullptr;

//     friend GlaxnimateGraphicsView;
};


class Registry
{
public:
    enum Group
    {
        Core,
        Draw,
        Shape,
        User
    };

    using container = std::map<int, std::multimap<Priority, std::unique_ptr<Tool>>>;
    using iterator = container::const_iterator;

    static Registry& instance()
    {
        static Registry instance;
        return instance;
    }

    iterator begin() const { return tools.begin(); }
    iterator end() const { return tools.end(); }

    void register_tool(int group, qreal priority, std::unique_ptr<Tool> tool)
    {
        tools[group].emplace(priority, std::move(tool));
    }


private:
    Registry() = default;
    Registry(const Registry&) = delete;
    ~Registry() = default;

    container tools;
};



template<class T>
class Autoreg
{
public:
    Autoreg(int group, Priority priority)
    {
        Registry::instance().register_tool(group, priority, std::make_unique<T>());
    }
};


} // namespace tools
