#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameLayoutData>
#include <QAbstractTextDocumentLayout>
#include <QInputDialog>

#include "draw_tool_base.hpp"
#include "model/shapes/text.hpp"
#include "widgets/tools/text_tool_widget.hpp"
#include "glaxnimate_app.hpp"


namespace tools {

class TextTool : public DrawToolBase
{
public:
    QCursor cursor() override { return Qt::IBeamCursor; }
    QString id() const override { return "text"; }
    QIcon icon() const override { return GlaxnimateApp::theme_icon("draw-text"); }
    QString name() const override { return QObject::tr("Draw Text"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F8"), QKeySequence::PortableText); }
    static int static_group() noexcept { return Registry::Shape; }
    int group() const noexcept override { return static_group(); }

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
                select(event, text);
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
        window = event.window;
    }

    void disable_event(const Event& event) override
    {
        commit(event);
        window = nullptr;
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
        target = nullptr;
        editor.setPlainText("");
        forward_click = false;
        modified = false;
    }

    void commit(const Event& event)
    {
        QString text = editor.toPlainText();

        if ( target )
        {
            if ( modified )
                target->text.set_undoable(text, true);
        }
        else if ( !text.isEmpty() )
        {
            auto shape = std::make_unique<model::TextShape>(event.window->document());
            shape->text.set(text);
            shape->name.set(text);
            shape->position.set(editor.pos() - editor_offet());
            shape->font->family.set(font.family());
            shape->font->style.set(font.styleName());
            shape->font->size.set(font.pointSizeF());
            create_shape(QObject::tr("Draw Text"), event, std::move(shape));
        }

        clear();
    }

    void select(const MouseEvent& event, model::TextShape* item)
    {
        select(event.scene, item);
        QPointF pos = editor.mapFromScene(event.scene_pos);
        QTextCursor cur(editor.document());
        int curpos = editor.document()->documentLayout()->hitTest(pos, Qt::FuzzyHit);
        cur.setPosition(curpos);
        editor.setTextCursor(cur);
    }

    void select(graphics::DocumentScene * scene, model::TextShape* item)
    {
        clear();

        widget()->set_font(item->font->query());

        scene->addItem(&editor);
        target = item;

        //         editor.setDefaultTextColor(Qt::transparent);
        editor.setPlainText(item->text.get());

        font = item->font->query();
        editor.setFont(font);
        editor.setFocus(Qt::OtherFocusReason);

        QPen pen(QColor(128, 0, 0, 100), 1);
        pen.setCosmetic(true);
        set_text_format(Qt::transparent, pen, item->font->line_spacing());

        editor.setPos({});
        update_editor_position();

        widget()->set_preview_text(target->text.get());
        show_keyboard();
    }

    void create(const MouseEvent& event)
    {
        clear();

        set_text_format(event.window->current_color(), event.window->current_pen_style(), base_line_spacing());
        editor.setTransform(QTransform{});
        editor.setPos(event.scene_pos + editor_offet());
        event.scene->addItem(&editor);
        editor.setPlainText("");
        editor.setFocus(Qt::OtherFocusReason);
        editor.setDefaultTextColor(Qt::black);
        editor.setFont(font);
        widget()->set_preview_text("");
        show_keyboard();
    }

    void show_keyboard()
    {
#ifdef Q_OS_ANDROID
        QGuiApplication::inputMethod()->show();
#endif
    }

    QWidget* on_create_widget() override
    {
        auto widget = new TextToolWidget();
        connect(widget, &TextToolWidget::checks_changed, this, &TextTool::update_format);
        return widget;
    }

private:
    void update_editor_position()
    {
        QTransform trans = target->transform_matrix(target->time());
        QPointF pos = target->position.get() + editor_offet();
        trans.translate(pos.x(), pos.y());
        editor.setTransform(trans);
    }

    TextToolWidget* widget()
    {
        return static_cast<TextToolWidget*>(get_settings_widget());
    }

    qreal base_line_spacing()
    {
        QFontMetrics metrics(editor.font());
        return metrics.ascent() + metrics.descent();
    }

    void set_text_format(const QBrush& fill, const QPen& stroke, qreal line_height)
    {
        editor.document()->setUseDesignMetrics(true);
        QTextCursor cur = editor.textCursor();
        cur.movePosition(QTextCursor::Start);
        cur.select(QTextCursor::Document);
        QTextCharFormat fmt;
        if ( widget()->create_stroke() )
            fmt.setTextOutline(stroke);
        if ( widget()->create_fill() )
        fmt.setForeground(fill);
        cur.setCharFormat(fmt);
        QTextBlockFormat bfmt;
        bfmt.setLineHeight(line_height - base_line_spacing(), QTextBlockFormat::LineDistanceHeight);
        cur.setBlockFormat(bfmt);
        editor.setTextCursor(cur);
    }

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
        if ( auto text = impl_extract_selection_recursive_item(scene, node) )
            select(scene, text);
    }

    void on_font_changed(const QFont& f)
    {
        font = f;
        editor.setFont(f);

        if ( target )
        {
            target->font->from_qfont(f);
            update_editor_position();
        }
    }

    void initialize(const Event&) override
    {
        editor.setTextInteractionFlags(Qt::TextEditorInteraction);
        editor.setZValue(9001);
        font = widget()->font();
        connect(widget(), &TextToolWidget::font_changed, this, &TextTool::on_font_changed);
        connect(editor.document(), &QTextDocument::contentsChanged, this, &TextTool::apply_changes);
    }

    void apply_changes()
    {
        if ( target )
        {
            QString text = editor.toPlainText();
            if ( text != target->text.get() )
            {
                target->text.set_undoable(text, false);
                modified = true;
            }
        }
    }

    void update_format()
    {
        if ( !target )
            set_text_format(window->current_color(), window->current_pen_style(), base_line_spacing());
    }

    static Autoreg<TextTool> autoreg;
    QGraphicsTextItem editor;
    model::TextShape* target = nullptr;
    bool forward_click = false;
    QFont font;
    bool modified = false;
    glaxnimate::gui::DocumentEnvironment* window = nullptr;
};

} // namespace tools

tools::Autoreg<tools::TextTool> tools::TextTool::autoreg{max_priority + 3};
