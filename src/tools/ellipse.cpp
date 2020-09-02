#include "base.hpp"

namespace tools {

class EllipseTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-ellipse"); }
    QString name() const override { return QObject::tr("Ellipse"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F5"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_move(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_release(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }
    QCursor cursor() override { return {}; }

private:
    static Autoreg<EllipseTool> autoreg;
};

} // namespace tools

tools::Autoreg<tools::EllipseTool> tools::EllipseTool::autoreg{tools::Registry::Shape, max_priority + 1};
