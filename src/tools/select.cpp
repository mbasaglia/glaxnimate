#include "base.hpp"

namespace tools {

class SelectTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("edit-select"); }
    QString name() const override { return QObject::tr("Select"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F1"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_move(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_release(const MouseEvent& event) override
    {
        for ( auto item : event.scene->nodes(event.scene_pos, event.view->transform()) )
            if ( item->node()->docnode_selectable() )
            {
                event.scene->user_select({item->node()}, !(event.modifiers() & (Qt::ShiftModifier|Qt::ControlModifier)) );
                break;
            }
    }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }

    static Autoreg<SelectTool> autoreg;
};

} // namespace tools


tools::Autoreg<tools::SelectTool> tools::SelectTool::autoreg{tools::Registry::Core, max_priority};
