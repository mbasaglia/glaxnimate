#include "base.hpp"

namespace tools {


class EditTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("edit-node"); }
    QString name() const override { return QObject::tr("Edit"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F2"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_move(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_release(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }

    static Autoreg<EditTool> autoreg;
};


class DrawTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-path"); }
    QString name() const override { return QObject::tr("Draw Bezier"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F3"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_move(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_release(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }

    static Autoreg<DrawTool> autoreg;
};


class FreehandTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-freehand"); }
    QString name() const override { return QObject::tr("Draw Freehand"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F6"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_move(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_release(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }

    static Autoreg<FreehandTool> autoreg;
};


class RectangleTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-rectangle"); }
    QString name() const override { return QObject::tr("Rectangle"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F4"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_move(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_release(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }

    static Autoreg<RectangleTool> autoreg;
};


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

    static Autoreg<EllipseTool> autoreg;
};


class StarTool : public Tool
{
public:
    QIcon icon() const override { return QIcon::fromTheme("draw-star"); }
    QString name() const override { return QObject::tr("Star"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("*"), QKeySequence::PortableText); }
    app::settings::SettingList settings() const override { return {}; }

private:
    void mouse_press(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_move(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_release(const MouseEvent& event) override { Q_UNUSED(event); }
    void mouse_double_click(const MouseEvent& event) override { Q_UNUSED(event); }
    void paint(const PaintEvent& event) override { Q_UNUSED(event); }

    static Autoreg<StarTool> autoreg;
};


} // namespace tools

tools::Autoreg<tools::EditTool> tools::EditTool::autoreg{tools::Registry::Core, max_priority + 1};

tools::Autoreg<tools::DrawTool> tools::DrawTool::autoreg{tools::Registry::Draw, max_priority};
tools::Autoreg<tools::FreehandTool> tools::FreehandTool::autoreg{tools::Registry::Draw, max_priority + 1};

tools::Autoreg<tools::RectangleTool> tools::RectangleTool::autoreg{tools::Registry::Shape, max_priority};
tools::Autoreg<tools::EllipseTool> tools::EllipseTool::autoreg{tools::Registry::Shape, max_priority + 1};
tools::Autoreg<tools::StarTool> tools::StarTool::autoreg{tools::Registry::Shape, max_priority + 2};
