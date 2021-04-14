#include <QGraphicsTextItem>

#include "draw_tool_base.hpp"
#include "model/shapes/text.hpp"

namespace tools {


class TextTool : public DrawToolBase
{
public:
    QCursor cursor() override { return Qt::IBeamCursor; }
    QString id() const override { return "draw-text"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-text"); }
    QString name() const override { return QObject::tr("Draw Text"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F8"), QKeySequence::PortableText); }

    void mouse_press(const MouseEvent& event) override
    {
        event.forward_to_scene();

        forward_click = editor.scene() && editor.mapToScene(editor.boundingRect()).containsPoint(event.scene_pos, Qt::WindingFill);
    }

    void mouse_move(const MouseEvent& event) override
    {
        event.forward_to_scene();
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( forward_click )
        {
            event.forward_to_scene();
            forward_click = false;
            return;
        }

        auto clicked_on = under_mouse(event, true, SelectionMode::Shape).nodes;
        for ( auto shape : clicked_on )
        {
            if ( auto text = shape->node()->cast<model::TextShape>() )
            {
                select(event, text, shape->parentItem());
                return;
            }
        }

        if ( editor.scene() )
            commit(event);
        else
            create(event);
    }

    void mouse_double_click(const MouseEvent& event) override
    {
        if ( !editor.scene() )
            return mouse_release(event);


        if ( !editor.mapToScene(editor.boundingRect()).containsPoint(event.scene_pos, Qt::WindingFill) )
            commit(event);
        else
            event.forward_to_scene();
    }

    void paint(const PaintEvent& event) override
    {
        Q_UNUSED(event);
    }

    void key_press(const KeyEvent& event) override
    {
        QCoreApplication::sendEvent(event.scene, event.event);
    }

    void key_release(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Escape )
        {
            commit(event);
            event.repaint();
            event.accept();
            event.window->switch_tool(Registry::instance().tool("select"));
        }
        else
        {
            QCoreApplication::sendEvent(event.scene, event.event);
        }
    }

    void enable_event(const Event& event) override
    {
        editor.setTextInteractionFlags(Qt::TextEditorInteraction);
        clear();
    }

    void disable_event(const Event& event) override
    {
        commit(event);
    }

    void clear()
    {
        if ( editor.scene() )
            editor.scene()->removeItem(&editor);
        editor.setPlainText("");
        target = nullptr;
        forward_click = false;
    }

    void commit(const Event& event)
    {
        QString text = editor.toPlainText();
        if ( !text.isEmpty() )
        {
            if ( !target )
            {
                auto shape = std::make_unique<model::TextShape>(event.window->document());
                shape->text.set(text);
                shape->name.set(text);
                shape->position.set(editor.pos());
                create_shape(QObject::tr("Draw Text"), event, std::move(shape));
            }
            else
            {
                target->text.set_undoable(text);
            }
        }

        clear();
    }

    void select(const Event& event, model::TextShape* item, QGraphicsItem* parent = nullptr)
    {
        clear();

        if ( !parent )
            parent = event.scene->item_from_node(item->docnode_parent());

        editor.setParentItem(parent);
        editor.setPos(item->position.get());
        editor.setPlainText(item->text.get());
        target = item;
    }

    void create(const MouseEvent& event)
    {
        clear();

        event.scene->addItem(&editor);
        editor.setPos(event.scene_pos);
        editor.setPlainText("");
        editor.setFocus(Qt::OtherFocusReason);
        editor.setDefaultTextColor(Qt::black);
    }

private:
    static Autoreg<TextTool> autoreg;
    QGraphicsTextItem editor;
    model::TextShape* target = nullptr;
    bool forward_click = false;
};

} // namespace tools

tools::Autoreg<tools::TextTool> tools::TextTool::autoreg{tools::Registry::Shape, max_priority + 3};
