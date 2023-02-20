/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include "base.hpp"
#include "math/bezier/point.hpp"

namespace glaxnimate::model {
class Styler;
} // namespace glaxnimate::model

namespace glaxnimate::gui::tools {

class EditTool : public Tool
{
    Q_OBJECT

public:
    EditTool();
    ~EditTool();

    QString id() const override { return "edit"; }
    QIcon icon() const override { return QIcon::fromTheme("edit-node"); }
    QString name() const override { return QObject::tr("Edit"); }
    QKeySequence key_sequence() const override { return QKeySequence(QObject::tr("F2"), QKeySequence::PortableText); }
    static int static_group() noexcept { return Registry::Core; }
    int group() const noexcept override { return static_group(); }

    void selection_set_vertex_type(math::bezier::PointType t);
    void selection_delete();
    void selection_straighten();
    void selection_curve();
    void selection_dissolve();

    void add_point_mode();

private:
    void mouse_press(const MouseEvent& event) override;
    void mouse_move(const MouseEvent& event) override;
    void mouse_release(const MouseEvent& event) override;
    void mouse_double_click(const MouseEvent& event) override;

    void paint(const PaintEvent& event) override;
    QCursor cursor() override;

    void key_press(const KeyEvent& event) override;
    void key_release(const KeyEvent& event) override;

    void on_selected(graphics::DocumentScene * scene, model::VisualNode * node) override;

    void enable_event(const Event&) override;
    void disable_event(const Event&) override;

    QWidget* on_create_widget() override;

    void set_cursor(Qt::CursorShape shape);

    void exit_add_point_mode();

signals:
    void gradient_stop_changed(model::Styler* styler, int stop);

private:
    class Private;
    std::unique_ptr<Private> d;

    static Autoreg<EditTool> autoreg;
};

} // namespace glaxnimate::gui::tools

