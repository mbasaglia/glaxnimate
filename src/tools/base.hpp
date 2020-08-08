#pragma once

#include <map>
#include <memory>

#include <QMouseEvent>
#include <QAction>

#include "model/graphics/document_scene.hpp"
#include "ui/widgets/glaxnimate_graphics_view.hpp"
#include "ui/widgets/scalable_button.hpp"
#include "ui/dialogs/glaxnimate_window.hpp"
#include "app/settings/setting.hpp"
#include "app/settings/widget_builder.hpp"


namespace tools {

struct MouseEvent
{
    QMouseEvent* event;
    QPointF scene_pos;
    GlaxnimateGraphicsView* view;
    model::graphics::DocumentScene* scene;
    GlaxnimateWindow* window;

    Qt::KeyboardModifiers modifiers() const { return event->modifiers(); }
    Qt::MouseButton button() const { return event->button(); }
    Qt::MouseButtons buttons() const { return event->buttons(); }
};

struct PaintEvent
{
    GlaxnimateGraphicsView* view;
    model::graphics::DocumentScene* scene;
    GlaxnimateWindow* window;

    QPainter* painter;
};

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
            QObject::connect(button, &QAbstractButton::clicked, action, &QAction::setChecked);
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

protected:
    virtual void mouse_press(const MouseEvent& event) = 0;
    virtual void mouse_move(const MouseEvent& event) = 0;
    virtual void mouse_release(const MouseEvent& event) = 0;
    virtual void mouse_double_click(const MouseEvent& event) = 0;
    virtual void paint(const PaintEvent& event) = 0;

    virtual QWidget* on_create_widget()
    {
        QWidget* widget = new QWidget();
        QFormLayout* layout = new QFormLayout(widget);
        app::settings::WidgetBuilder().add_widgets(settings(), widget, layout, settings_values);
        return widget;
    }

    virtual void on_translate() {}


    QVariantMap settings_values;

private:
    QAction* action = nullptr;
    ScalableButton* button = nullptr;
    QWidget* settings_widget = nullptr;

    friend GlaxnimateGraphicsView;
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

    using Priority = int;

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
    Autoreg(int group, Registry::Priority priority)
    {
        Registry::instance().register_tool(group, priority, std::make_unique<T>());
    }
};


} // namespace tools
