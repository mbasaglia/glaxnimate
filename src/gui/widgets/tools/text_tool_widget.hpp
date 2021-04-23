#pragma once

#include "model/shapes/text.hpp"

#include "shape_tool_widget.hpp"

class TextToolWidget : public ShapeToolWidget
{
    Q_OBJECT

public:
    TextToolWidget(QWidget* parent = nullptr);

    QFont font() const;
    void set_font(const QFont& font);

    void set_preview_text(const QString& text);

signals:
    void font_changed(const QFont& font);

private:
    void on_font_changed();
    class Private;
    Private* dd() const;
};

