#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include <memory>
#include <QWidget>

namespace color_widgets {
class ColorPaletteModel;
} // namespace color_widgets

namespace model {
class Document;
class BrushStyle;
class NamedColor;
} // namespace model

class ColorSelector : public QWidget
{
    Q_OBJECT

public:
    ColorSelector(QWidget* parent = nullptr);
    ~ColorSelector();

    void hide_secondary();

    QColor current_color() const;
    QColor secondary_color() const;

    void save_settings();

    void set_document(model::Document* document);

    void set_palette_model(color_widgets::ColorPaletteModel* palette_model);

public slots:
    void set_current_color(const QColor& c);
    void set_secondary_color(const QColor& c);

private slots:
    void color_update_noalpha(const QColor& col);
    void color_update_alpha(const QColor& col);
    void color_update_component(int value);
    void color_swap();
    void commit_current_color();
    void swatch_give_color();

signals:
    void current_color_changed(const QColor& c);
    void secondary_color_changed(const QColor& c);
    void current_color_committed(const QColor& c);
    void current_color_def(model::BrushStyle* def);

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // COLORSELECTOR_H
