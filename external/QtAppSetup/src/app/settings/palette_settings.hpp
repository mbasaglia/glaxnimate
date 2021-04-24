#pragma once

#include "custom_settings_group.hpp"

#include <QPalette>
#include <QMap>

namespace app::settings {

class PaletteSettings : public CustomSettingsGroupBase
{
public:
    class Palette : public QPalette
    {
    public:
        Palette() = default;
        Palette(const QPalette& oth, bool built_in = false) : QPalette(oth), built_in(built_in) {}
        Palette(QPalette&& oth, bool built_in = false) : QPalette(std::move(oth)), built_in(built_in) {}
        Palette(const Palette& oth) : QPalette(oth), built_in(oth.built_in) {}
        Palette(Palette&& oth) : QPalette(std::move(oth)), built_in(oth.built_in) {}
        Palette& operator=(const Palette& oth)
        {
            QPalette::operator=(oth);
            built_in = oth.built_in;
            return *this;
        }
        Palette& operator=(Palette&& oth)
        {
            QPalette::operator=(std::move(oth));
            built_in = oth.built_in;
            return *this;
        }
        Palette& operator=(const QPalette& oth)
        {
            QPalette::operator=(oth);
            return *this;
        }
        Palette& operator=(QPalette&& oth)
        {
            QPalette::operator=(std::move(oth));
            return *this;
        }

        bool built_in = false;
    };

    PaletteSettings();

    QString slug() const override;
    QString label() const override;
    QIcon icon() const override;
    void load ( QSettings & settings ) override;
    void save ( QSettings & settings ) override;
    QWidget * make_widget ( QWidget * parent ) override;

    void load_palette(const QSettings& settings, bool mark_built_in = false);
    void write_palette(QSettings& settings, const QString& name, const QPalette& palette);

    const QPalette& palette() const;

    static const std::vector<std::pair<QString, QPalette::ColorRole>>& roles();

    void set_selected(const QString& name);
    void apply_palette(const QPalette& palette);
    void set_style(const QString& name);

    static QString color_to_string(const QColor& c);
    static QColor string_to_color(const QString& s);

    QMap<QString, Palette> palettes;
    QString selected;
    Palette default_palette;
    QString style;
};

} // namespace app::settings
