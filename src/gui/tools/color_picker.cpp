#include "base.hpp"

#include <QtColorWidgets/color_utils.hpp>
#include "widgets/tools/color_picker_widget.hpp"

namespace tools {

class FreehandTool : public Tool
{
public:
    QString id() const override { return "color-picker"; }
    QIcon icon() const override { return QIcon::fromTheme("color-picker"); }
    QString name() const override { return QObject::tr("Color Picker"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F7"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

    void mouse_move(const MouseEvent& event) override
    {
        widget()->set_color(color_widgets::utils::get_screen_color(event.event->globalPos()));
    }

    void mouse_release(const MouseEvent& event) override
    {
        QColor color = color_widgets::utils::get_screen_color(event.event->globalPos());
        widget()->set_color(color);
        if ( widget()->set_fill() )
            event.window->set_current_color(color);
        else
            event.window->set_secondary_color(color);
    }
    void key_press(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Shift )
            widget()->swap_fill_color();
    }
    void key_release(const KeyEvent& event) override
    {
        if ( event.key() == Qt::Key_Shift )
            widget()->swap_fill_color();
    }

    QCursor cursor() override { return Qt::CrossCursor; }

    bool show_editors(model::DocumentNode*) const override { return false; }

    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }
    void enable_event(const Event& event) override { Q_UNUSED(event); }
    void disable_event(const Event& event) override { Q_UNUSED(event); }

protected:
    QWidget* on_create_widget() override
    {
        return new ColorPickerWidget();
    }

    ColorPickerWidget* widget()
    {
        return static_cast<ColorPickerWidget*>(get_settings_widget());
    }

private:
    bool grabbing = false;
    static Autoreg<FreehandTool> autoreg;
};


} // namespace tools


tools::Autoreg<tools::FreehandTool> tools::FreehandTool::autoreg{tools::Registry::Style, max_priority};

