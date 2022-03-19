#include <QPointer>

#include "base.hpp"
#include "widgets/tools/fill_tool_widget.hpp"

#include "glaxnimate/core/model/shapes/fill.hpp"
#include "glaxnimate/core/model/shapes/stroke.hpp"
#include "glaxnimate/core/command/undo_macro_guard.hpp"
#include "glaxnimate/core/command/shape_commands.hpp"


namespace glaxnimate::gui::tools {

class FillTool : public Tool
{
public:
    QString id() const override { return "fill"; }
    QIcon icon() const override { return QIcon::fromTheme("fill-color"); }
    QString name() const override { return QObject::tr("Fill"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F8"), QKeySequence::PortableText); }
    static int static_group() noexcept { return Registry::Style; }
    int group() const noexcept override { return static_group(); }

    void mouse_move(const MouseEvent& event) override
    {
        auto um = under_mouse(event, true, SelectionMode::Shape);
        highlight = nullptr;
        for ( auto node : um.nodes )
        {
            if ( auto path = node->node()->cast<model::Shape>() )
            {
                if ( !event.scene->has_editors(path) )
                    highlight = path;
                break;
            }
        }
    }

    void mouse_release(const MouseEvent& event) override
    {
        if ( !highlight )
            return;

        auto document = event.window->document();
        model::ShapeListProperty* prop = highlight->owner();

        command::UndoMacroGuard guard(tr("Apply Style"), document);

        bool add_fill = widget()->fill();
        bool add_stroke = widget()->stroke();

        for ( const auto& se : *prop )
        {
            if ( add_fill )
            {
                if ( auto fill = se->cast<model::Fill>() )
                {
                    fill->use.set_undoable(QVariant::fromValue((model::BrushStyle*)nullptr));
                    fill->color.set_undoable(event.window->current_color());
                    add_fill = false;
                    continue;
                }
            }

            if ( add_stroke )
            {
                if ( auto stroke = se->cast<model::Stroke>() )
                {
                    stroke->use.set_undoable(QVariant::fromValue((model::BrushStyle*)nullptr));
                    stroke->set_pen_style_undoable(event.window->current_pen_style());
                    add_stroke = false;
                    continue;
                }
            }
        }

        int index = prop->index_of(highlight.data());

        if ( add_fill )
        {
            auto fill = std::make_unique<model::Fill>(document);
            document->set_best_name(fill.get(), QObject::tr("%1 Fill").arg(highlight->name.get()));
            fill->color.set(event.window->current_color());

            document->undo_stack().push(
                new command::AddShape(prop, std::move(fill), index)
            );
        }

        if ( add_stroke )
        {
            auto stroke = std::make_unique<model::Stroke>(document);
            document->set_best_name(stroke.get(), QObject::tr("%1 Stroke").arg(highlight->name.get()));
            stroke->set_pen_style(event.window->current_pen_style());

            document->undo_stack().push(
                new command::AddShape(prop, std::move(stroke), index)
            );
        }

        event.window->set_current_document_node(highlight->docnode_visual_parent());

        highlight = nullptr;
    }

    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Shift )
        {
            widget()->swap_fill_color();
            event.repaint();
        }
    }
    void key_release(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Shift )
        {
            widget()->swap_fill_color();
            event.repaint();
        }
    }

    QCursor cursor() override { return Qt::CrossCursor; }

    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }

    void paint(const PaintEvent& event) override
    {
        if ( highlight )
        {
            QPainterPath p;
            highlight->to_bezier(highlight->time()).add_to_painter_path(p);
            QTransform trans = highlight->transform_matrix(highlight->time()) * event.view->viewportTransform();
            p = trans.map(p);
            event.painter->setPen(widget()->stroke() ? event.window->current_pen_style() : Qt::NoPen);
            event.painter->setBrush(widget()->fill() ? event.window->current_color() : Qt::transparent);
            event.painter->drawPath(p);
        }
    }

    void enable_event(const Event&) override
    {
        highlight = nullptr;
    }

    void disable_event(const Event&) override
    {
        highlight = nullptr;
    }

protected:
    QWidget* on_create_widget() override
    {
        return new FillToolWidget();
    }

    FillToolWidget* widget()
    {
        return static_cast<FillToolWidget*>(get_settings_widget());
    }

private:
    QPointer<model::Shape> highlight;

    static Autoreg<FillTool> autoreg;
};


tools::Autoreg<tools::FillTool> tools::FillTool::autoreg{max_priority+1};

} // namespace glaxnimate::gui::tools

