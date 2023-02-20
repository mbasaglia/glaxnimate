#pragma once

#include <map>
#include <memory>

#include <QMouseEvent>
#include <QKeyEvent>
#include <QAction>

#include "glaxnimate_app.hpp"
#include "graphics/document_scene.hpp"
#include "graphics/document_node_graphics_item.hpp"
#include "graphics/handle.hpp"
#include "widgets/canvas.hpp"
#include "widgets/scalable_button.hpp"
#include "widgets/dialogs/selection_manager.hpp"

namespace glaxnimate::gui::tools {

struct Event
{
    Canvas* view;
    graphics::DocumentScene* scene;
    glaxnimate::gui::SelectionManager* window;

    void repaint() const
    {
        view->viewport()->update();
    }
};

struct MouseEvent : Event
{
    /// Originating Qt event
    QMouseEvent* event;
    /// Mouse position in scene coordinates
    QPointF scene_pos;

    /// Mouse press that started the event (also available in move events)
    Qt::MouseButton press_button;
    /// Position of when the button has been pressed (scene coordinates)
    QPointF press_scene_pos;
    /// Position of when the button has been pressed (screen coordinates)
    QPoint press_screen_pos;
    /// Position of the last known mouse position (scene coordinates)
    QPointF last_scene_pos;
    /// Position of the last known mouse position (screen coordinates)
    QPoint last_screen_pos;

    /// Modifiers being held during the event
    Qt::KeyboardModifiers modifiers() const { return event->modifiers(); }
    /// Button that triggered press/release
    Qt::MouseButton button() const { return event->button(); }
    /// Buttons being held during the event
    Qt::MouseButtons buttons() const { return event->buttons(); }
    /// Position of the event in view coordinates
#if QT_VERSION_MAJOR < 6
    const QPointF& pos() const { return event->localPos(); }
#else
    QPointF pos() const { return event->position(); }
#endif
    /// Tell the Qt event that it should not propagate
    void accept() const { event->accept(); }

    /// Use the default behaviour for this event (useful to select items etc)
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

class Tool : public QObject
{
    Q_OBJECT

public:
    using SelectionMode = graphics::DocumentNodeGraphicsItem::SelectionMode;

    virtual ~Tool() = default;

    virtual QString id() const = 0;
    virtual QIcon icon() const = 0;
    virtual QString name() const = 0;
    virtual QString tooltip() const { return name(); }
    virtual QKeySequence key_sequence() const = 0;

    QAction* get_action();

    /**
     * \pre get_action() called before calling this
     */
    ScalableButton* get_button();

    QWidget* get_settings_widget();

    /**
     * \pre get_action and get_button already called
     */
    void retranslate();

    virtual void mouse_press(const MouseEvent& event) = 0;
    virtual void mouse_move(const MouseEvent& event) = 0;
    virtual void mouse_release(const MouseEvent& event) = 0;
    virtual void mouse_double_click(const MouseEvent& event) = 0;
    virtual void paint(const PaintEvent& event) = 0;
    virtual void key_press(const KeyEvent& event) = 0;
    virtual void key_release(const KeyEvent& event) { Q_UNUSED(event); }
    virtual QCursor cursor() = 0;
    virtual void enable_event(const Event& event) = 0;
    virtual void disable_event(const Event& event) = 0;
    virtual void close_document_event(const Event& event) { Q_UNUSED(event); }
    virtual void shape_style_change_event(const Event& event) { Q_UNUSED(event); }

    virtual void on_selected(graphics::DocumentScene* scene, model::VisualNode* node) { Q_UNUSED(scene); Q_UNUSED(node); }
    virtual void on_deselected(graphics::DocumentScene* scene, model::VisualNode* node);
    // Brief called once the GUI has been fully initialized
    virtual void initialize(const Event& event) { Q_UNUSED(event); }

    virtual int group() const noexcept = 0;

protected:
    struct UnderMouse
    {
        graphics::MoveHandle* handle = nullptr;
        std::vector<graphics::DocumentNodeGraphicsItem*> nodes;
    };

    UnderMouse under_mouse(const MouseEvent& event, bool only_selectable, SelectionMode mode) const;
    graphics::MoveHandle* handle_under_mouse(const MouseEvent& event) const;

    virtual QWidget* on_create_widget() = 0;
    virtual void on_translate() {}

    QVariantMap settings_values;

    static constexpr Priority max_priority = std::numeric_limits<Priority>::min();

    void edit_clicked(const MouseEvent& event);

signals:
    void cursor_changed(const QCursor&);

private:
    QAction* action = nullptr;
    ScalableButton* button = nullptr;
    QWidget* settings_widget = nullptr;

//     friend Canvas;
};


class Registry
{
public:
    enum Group
    {
        Core,
        Draw,
        Shape,
        Style,
        User
    };

    using container = std::map<int, std::multimap<Priority, std::unique_ptr<Tool>>>;
    using iterator = container::const_iterator;
    using mapped_type = container::mapped_type;

    static Registry& instance()
    {
        static Registry instance;
        return instance;
    }

    iterator begin() const { return tools.begin(); }
    iterator end() const { return tools.end(); }

    void register_tool(int group, qreal priority, std::unique_ptr<Tool> tool)
    {
        by_id[tool->id()] = tool.get();
        tools[group].emplace(priority, std::move(tool));
    }

    Tool* tool(const QString& id) const
    {
        auto it = by_id.find(id);
        if ( it == by_id.end() )
            return nullptr;
        return it->second;
    }

    const mapped_type& operator[](Group g) const
    {
        return tools.at(g);
    }

private:
    Registry() = default;
    Registry(const Registry&) = delete;
    ~Registry() = default;

    container tools;
    std::map<QString, Tool*> by_id;
};



template<class T>
class Autoreg
{
public:
    Autoreg(Priority priority)
    {
        Registry::instance().register_tool(T::static_group(), priority, std::make_unique<T>());
    }
};


} // namespace glaxnimate::gui::tools
