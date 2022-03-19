#pragma once

#include "glaxnimate/core/model/shapes/text.hpp"

#include "shape_tool_widget.hpp"

namespace glaxnimate::gui {

class TextToolWidget : public ShapeToolWidget
{
    Q_OBJECT

public:
    TextToolWidget(QWidget* parent = nullptr);

    QFont font() const;
    void set_font(const QFont& font);
    void set_document(model::Document* document);

    void set_preview_text(const QString& text);

signals:
    void font_changed(const QFont& font);
    void custom_font_selected(int database_index);

private:
    void on_font_changed();
    class Private;
    Private* dd() const;
};

} // namespace glaxnimate::gui
