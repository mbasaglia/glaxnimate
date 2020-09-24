#include "color_selector.hpp"
#include "ui_color_selector.h"

#include "QtColorWidgets/color_palette_model.hpp"
#include "app/application.hpp"
#include "app/settings/settings.hpp"

#include "model/defs/defs.hpp"
#include "model/document.hpp"
#include "command/object_list_commands.hpp"
#include "command/animation_commands.hpp"
#include "utils/pseudo_mutex.hpp"

namespace {


void color_hsv_h(QColor& c, int v) { c.setHsv(v, c.hsvSaturation(), c.value(), c.alpha()); }
void color_hsv_s(QColor& c, int v) { c.setHsv(c.hsvHue(), v, c.value(), c.alpha()); }
void color_hsv_v(QColor& c, int v) { c.setHsv(c.hsvHue(), c.hsvSaturation(), v, c.alpha()); }

void color_hsl_h(QColor& c, int v) { c.setHsv(v, c.hslSaturation(), c.value(), c.alpha()); }
void color_hsl_s(QColor& c, int v) { c.setHsl(c.hslHue(), v, c.lightness(), c.alpha()); }
void color_hsl_l(QColor& c, int v) { c.setHsl(c.hslHue(), c.hslSaturation(), v, c.alpha()); }

void color_r(QColor& c, int v) { c.setRed(v); }
void color_g(QColor& c, int v) { c.setGreen(v); }
void color_b(QColor& c, int v) { c.setBlue(v); }
void color_a(QColor& c, int v) { c.setAlpha(v); }

void color_c(QColor& c, int v) { c.setCmyk(v, c.magenta(), c.yellow(), c.black(), c.alpha()); }
void color_m(QColor& c, int v) { c.setCmyk(c.cyan(), v, c.yellow(), c.black(), c.alpha()); }
void color_y(QColor& c, int v) { c.setCmyk(c.cyan(), c.magenta(), v, c.black(), c.alpha()); }
void color_k(QColor& c, int v) { c.setCmyk(c.cyan(), c.magenta(), c.yellow(), v, c.alpha()); }

} // namespace

class ColorSelector::Private
{
public:
    bool updating_color = false;
    Ui::ColorSelector ui;
    color_widgets::ColorPaletteModel palette_model;
    ColorSelector* parent;
    model::Document* document = nullptr;
    utils::PseudoMutex updating_swatch;

    void setup_ui(ColorSelector* parent)
    {
        this->parent = parent;
        ui.setupUi(parent);

        update_color(QColor(app::settings::get<QString>("tools", "color_main")), true, nullptr);
        ui.palette_widget->setModel(&palette_model);
        palette_model.setSearchPaths(app::Application::instance()->data_paths_unchecked("palettes"));
        ui.color_preview_secondary->setColor(QColor(app::settings::get<QString>("tools", "color_secondary")));
        connect(ui.color_preview_secondary, &color_widgets::ColorSelector::colorSelected, parent, &ColorSelector::secondary_color_changed);

        for ( auto slider : parent->findChildren<QSlider*>() )
            connect(slider, &QSlider::sliderReleased, parent, &ColorSelector::commit_current_color);
        for ( auto spin : parent->findChildren<QSpinBox*>() )
            connect(spin, &QSpinBox::editingFinished, parent, &ColorSelector::commit_current_color);
        for ( auto wheel : parent->findChildren<color_widgets::ColorWheel*>() )
            connect(wheel, &color_widgets::ColorWheel::editingFinished, parent, &ColorSelector::commit_current_color);
    }

    void update_color_slider(color_widgets::GradientSlider* slider, const QColor& c,
                             void (*func)(QColor&, int), int val, int min = 0, int max = 255);
    void update_color_hue_slider(color_widgets::HueSlider* slider, const QColor& c, int hue);
    QColor current_color();
    QColor current_color_secondary();
    void set_current_color(const QColor c);
    void set_current_color_secondary(const QColor c);
    void update_color(const QColor& c, bool alpha, QObject* source);
    void update_color_component(int val, QObject* sender);
    void color_swap();
};


void ColorSelector::Private::update_color_slider(color_widgets::GradientSlider* slider, const QColor& c,
                            void (*func)(QColor&, int), int val, int min, int max)
{
    QColor c1 = c;
    (*func)(c1, min);
    QColor c2 = c;
    (*func)(c2, (min+max)/2);
    QColor c3 = c;
    (*func)(c3, max);
    slider->setColors({c1, c2, c3});
    slider->setValue(val);
}

void ColorSelector::Private::update_color_hue_slider(color_widgets::HueSlider* slider, const QColor& c, int hue)
{
    slider->setColorSaturation(c.saturationF());
    slider->setColorValue(c.valueF());
    slider->setValue(hue);
}


QColor ColorSelector::Private::current_color()
{
    return ui.color_preview->color();
}

QColor ColorSelector::Private::current_color_secondary()
{
    return ui.color_preview_secondary->color();
}

void ColorSelector::Private::set_current_color(const QColor c)
{
    update_color(c, true, nullptr);
}

void ColorSelector::Private::set_current_color_secondary(const QColor c)
{
    ui.color_preview_secondary->setColor(c);
}

void ColorSelector::Private::update_color(const QColor& c, bool alpha, QObject* source)
{
    if ( updating_color )
        return;
    updating_color = true;

    QColor col = c;
    if ( !alpha )
        col.setAlpha(current_color().alpha());

    int hue = col.hsvHue();
    if ( hue == -1 )
        hue = col.hslHue();
    if ( hue == -1 )
        hue = current_color().hue();
    if ( hue == -1 )
        hue = 0;

    // main
    ui.color_preview->setColor(col);
    ui.color_line_edit->setColor(col);
    update_color_slider(ui.slider_alpha, col, color_a, col.alpha());

    // HSV
    if ( source != ui.color_hsv )
        ui.color_hsv->setColor(col);
    update_color_hue_slider(ui.slider_hsv_hue, col, hue);
    update_color_slider(ui.slider_hsv_sat, col, color_hsv_s, col.hsvSaturation());
    update_color_slider(ui.slider_hsv_value, col, color_hsv_v, col.value());

    // HSL
    if ( source != ui.color_hsl )
        ui.color_hsl->setColor(col);
    update_color_hue_slider(ui.slider_hsl_hue, col, hue);
    update_color_slider(ui.slider_hsl_sat, col, color_hsl_s, col.hslSaturation());
    update_color_slider(ui.slider_hsl_light, col, color_hsl_l, col.lightness());

    // RGB
    update_color_slider(ui.slider_rgb_r, col, color_r, col.red());
    update_color_slider(ui.slider_rgb_g, col, color_g, col.green());
    update_color_slider(ui.slider_rgb_b, col, color_b, col.blue());

    // CMYK
    update_color_slider(ui.slider_cmyk_c, col, color_c, col.cyan());
    update_color_slider(ui.slider_cmyk_m, col, color_m, col.magenta());
    update_color_slider(ui.slider_cmyk_y, col, color_y, col.yellow());
    update_color_slider(ui.slider_cmyk_k, col, color_k, col.black());

    // Palette
    if ( source != ui.palette_widget )
        ui.palette_widget->setCurrentColor(col);
    ui.palette_widget->setDefaultColor(col);

    updating_color = false;
    emit parent->current_color_changed(col);
}

void ColorSelector::Private::update_color_component(int val, QObject* sender)
{
    if ( updating_color )
        return;

    void (*func)(QColor&, int) = nullptr;

    if ( sender->objectName() == "slider_alpha" )
        func = color_a;
    else if ( sender->objectName() == "slider_hsv_hue" )
        func = color_hsv_h;
    else if ( sender->objectName() == "slider_hsv_sat" )
        func = color_hsv_s;
    else if ( sender->objectName() == "slider_hsv_value" )
        func = color_hsv_v;
    else if ( sender->objectName() == "slider_hsl_hue" )
        func = color_hsl_h;
    else if ( sender->objectName() == "slider_hsl_sat" )
        func = color_hsl_s;
    else if ( sender->objectName() == "slider_hsl_light" )
        func = color_hsl_l;
    else if ( sender->objectName() == "slider_rgb_r" )
        func = color_r;
    else if ( sender->objectName() == "slider_rgb_g" )
        func = color_g;
    else if ( sender->objectName() == "slider_rgb_b" )
        func = color_b;
    else if ( sender->objectName() == "slider_cmyk_c" )
        func = color_c;
    else if ( sender->objectName() == "slider_cmyk_m" )
        func = color_m;
    else if ( sender->objectName() == "slider_cmyk_y" )
        func = color_y;
    else if ( sender->objectName() == "slider_cmyk_k" )
        func = color_k;

    if ( !func )
        return;

    QColor c = current_color();
    (*func)(c, val);
    update_color(c, true, sender);
}


void ColorSelector::Private::color_swap()
{
    QColor c = ui.color_preview_secondary->color();
    ui.color_preview_secondary->setColor(current_color());
    ui.color_preview->setColor(c);
    update_color(c, true, nullptr);
    emit parent->secondary_color_changed(current_color_secondary());
}

ColorSelector::ColorSelector(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->setup_ui(this);

    auto palette = &d->ui.swatch->palette();
    connect(palette, &color_widgets::ColorPalette::colorAdded, this, &ColorSelector::swatch_palette_color_added);
    connect(palette, &color_widgets::ColorPalette::colorRemoved, this, &ColorSelector::swatch_palette_color_removed);
    connect(palette, &color_widgets::ColorPalette::colorChanged, this, &ColorSelector::swatch_palette_color_changed);
}

ColorSelector::~ColorSelector() {}

void ColorSelector::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void ColorSelector::save_settings()
{
    app::settings::set("tools", "color_main", d->current_color().name());
    app::settings::set("tools", "color_secondary", d->current_color_secondary().name());
}


void ColorSelector::color_swap()
{
    d->color_swap();
}

void ColorSelector::color_update_alpha ( const QColor& col )
{
    d->update_color(col, true, QObject::sender());
    emit current_color_committed(col);
}

void ColorSelector::color_update_noalpha ( const QColor& col )
{
    d->update_color(col, false, QObject::sender());
}

void ColorSelector::color_update_component ( int value )
{
    d->update_color_component(value, QObject::sender());
}

QColor ColorSelector::current_color() const
{
    return d->current_color();
}

QColor ColorSelector::secondary_color() const
{
    return d->current_color_secondary();
}

void ColorSelector::set_current_color(const QColor& c)
{
    d->set_current_color(c);
}

void ColorSelector::set_secondary_color(const QColor& c)
{
    d->set_current_color_secondary(c);
}

void ColorSelector::hide_secondary()
{
    d->ui.color_preview_secondary->hide();
    d->ui.color_swap->hide();
}

void ColorSelector::commit_current_color()
{
    emit current_color_committed(d->current_color());
}

void ColorSelector::set_document(model::Document* document)
{
    auto palette = &d->ui.swatch->palette();

    if ( d->document )
    {
        disconnect(d->document->defs(), nullptr, this, nullptr);
    }

    d->document = document;

    if ( d->document )
    {
        auto l = d->updating_swatch.get_lock();
        palette->setColors(QVector<QColor>{});
        for ( const auto& col : d->document->defs()->colors )
            palette->appendColor(col->color.get(), col->name.get());

        connect(d->document->defs(), &model::Defs::color_added, this, &ColorSelector::swatch_doc_color_added);
        connect(d->document->defs(), &model::Defs::color_removed, this, &ColorSelector::swatch_doc_color_removed);
        connect(d->document->defs(), &model::Defs::color_changed, this, &ColorSelector::swatch_doc_color_changed);
    }
}

void ColorSelector::swatch_palette_color_added(int index)
{
    if ( !d->document )
        return;

    if ( auto l = d->updating_swatch.get_lock() )
    {
        auto defs = d->document->defs();
        auto palette = &d->ui.swatch->palette();

        auto col = std::make_unique<model::NamedColor>(d->document);
        col->color.set(palette->colorAt(index));
        col->name.set(palette->nameAt(index));
        defs->push_command(new command::AddObject<model::NamedColor>(&defs->colors, std::move(col), index));
    }
}

void ColorSelector::swatch_palette_color_removed(int index)
{
    if ( !d->document )
        return;

    if ( auto l = d->updating_swatch.get_lock() )
    {
        auto defs = d->document->defs();

        if ( defs->colors.valid_index(index) )
            defs->push_command(new command::RemoveObject<model::NamedColor>(index, &defs->colors));
    }
}

void ColorSelector::swatch_palette_color_changed(int index)
{
    if ( !d->document )
        return;

    if ( auto l = d->updating_swatch.get_lock() )
    {

        auto defs = d->document->defs();
        auto palette = &d->ui.swatch->palette();

        if ( !defs->colors.valid_index(index) )
            return;

        d->document->undo_stack().beginMacro(tr("Modify Palette Color"));
        auto color = &defs->colors[index];
        color->name.set_undoable(palette->nameAt(index));
        color->color.set_undoable(palette->colorAt(index));
        d->document->undo_stack().endMacro();
    }
}

void ColorSelector::swatch_add()
{
    d->ui.swatch->palette().appendColor(d->current_color());
    swatch_link(d->ui.swatch->palette().count() - 1);
}

void ColorSelector::swatch_link(int index)
{
    if ( d->document && index != -1 )
        emit current_color_def(&d->document->defs()->colors[index]);
}

void ColorSelector::swatch_unlink()
{
    emit current_color_def(nullptr);
}


void ColorSelector::swatch_doc_color_added(int position, model::NamedColor* color)
{
    if ( auto l = d->updating_swatch.get_lock() )
    {
        d->ui.swatch->palette().insertColor(position, color->color.get(), color->name.get());
    }
}

void ColorSelector::swatch_doc_color_removed(int pos)
{
    if ( auto l = d->updating_swatch.get_lock() )
    {
        d->ui.swatch->palette().eraseColor(pos);
    }
}

void ColorSelector::swatch_doc_color_changed(int position, model::NamedColor* color)
{
    if ( auto l = d->updating_swatch.get_lock() )
    {
        d->ui.swatch->palette().setColorAt(position, color->color.get(), color->name.get());
    }
}
