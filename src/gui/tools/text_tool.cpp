#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameLayoutData>

#include "draw_tool_base.hpp"
#include "model/shapes/text.hpp"

namespace tools {


class TextTool : public DrawToolBase
{
public:
    QCursor cursor() override { return Qt::IBeamCursor; }
    QString id() const override { return "text"; }
    QIcon icon() const override { return QIcon::fromTheme("draw-text"); }
    QString name() const override { return QObject::tr("Draw Text"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F8"), QKeySequence::PortableText); }

    void mouse_press(const MouseEvent& event) override
    {
        forward_click = editor.scene() && editor.mapToScene(editor.boundingRect()).containsPoint(event.scene_pos, Qt::WindingFill);
        if ( forward_click )
            event.forward_to_scene();
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
                select(event.scene, text);
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
        {
            commit(event);
        }
        else
        {
            event.forward_to_scene();
            forward_click = true;
        }
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
        Q_UNUSED(event);
        initialize();
    }

    void disable_event(const Event& event) override
    {
        commit(event);
    }

    void close_document_event(const Event& event) override
    {
        Q_UNUSED(event);
        clear();
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
                shape->position.set(editor.pos() - editor_offet());
                create_shape(QObject::tr("Draw Text"), event, std::move(shape));
            }
            else
            {
                target->text.set_undoable(text);
            }
        }

        clear();
    }

    void select(graphics::DocumentScene * scene, model::TextShape* item)
    {
        clear();

        scene->addItem(&editor);
        target = item;
        editor.setPlainText(item->text.get());
        font = item->font->query();
        editor.setFont(font);
        editor.setFocus(Qt::OtherFocusReason);

        editor.setPos({});
        QTransform trans = item->transform_matrix(item->time());
        QPointF pos = item->position.get() + editor_offet();
        trans.translate(pos.x(), pos.y());
        editor.setTransform(trans);
    }

    void create(const MouseEvent& event)
    {
        clear();

        editor.setTransform(QTransform{});
        editor.setPos(event.scene_pos + editor_offet());
        event.scene->addItem(&editor);
        editor.setPlainText("");
        editor.setFocus(Qt::OtherFocusReason);
        editor.setDefaultTextColor(Qt::black);
        editor.setFont(font);
    }

private:
    QPointF editor_offet() const
    {
        auto fmt = editor.document()->rootFrame()->frameFormat();
        QFontMetrics metrics(font);
        auto margin = fmt.border() + fmt.padding();
        return QPointF(-margin - fmt.leftMargin(), -margin - fmt.topMargin() - metrics.ascent());
    }

    model::TextShape* impl_extract_selection_recursive_item(graphics::DocumentScene * scene, model::VisualNode* node)
    {
        auto meta = node->metaObject();
        if ( meta->inherits(&model::TextShape::staticMetaObject) )
            return static_cast<model::TextShape*>(node);

        if ( meta->inherits(&model::Group::staticMetaObject) )
        {
            for ( const auto& sub : static_cast<model::Group*>(node)->shapes )
            {
                if ( auto tn = impl_extract_selection_recursive_item(scene, sub.get()) )
                    return tn;
            }
        }

        return nullptr;
    }

    void on_selected(graphics::DocumentScene * scene, model::VisualNode * node) override
    {
        initialize();
        if ( auto text = impl_extract_selection_recursive_item(scene, node) )
            select(scene, text);
    }

    void initialize()
    {
        if ( !initialized )
        {
            initialized = true;
            editor.setTextInteractionFlags(Qt::TextEditorInteraction);
            editor.setZValue(9001);
            font = QFont("sans", 32);
        }
    }

    static Autoreg<TextTool> autoreg;
    QGraphicsTextItem editor;
    model::TextShape* target = nullptr;
    bool forward_click = false;
    QFont font;
    bool initialized = false;
};

} // namespace tools

tools::Autoreg<tools::TextTool> tools::TextTool::autoreg{tools::Registry::Shape, max_priority + 3};
