#pragma once

#include "custom_settings_group.hpp"

#include <QPalette>
#include <QMap>

namespace app::settings {

class PaletteSettings : public CustomSettingsGroupBase
{
public:
    PaletteSettings();

    QString slug() const override;
    QString label() const override;
    QIcon icon() const override;
    void load ( QSettings & settings ) override;
    void save ( QSettings & settings ) override;
    QWidget * make_widget ( QWidget * parent ) override;

    void load_palette(const QSettings& settings);
    void write_palette(QSettings& settings, const QString& name, const QPalette& palette);

    const QPalette& palette() const;

    static const std::vector<std::pair<QString, QPalette::ColorRole>>& roles();

    void set_selected(const QString& name);
    void apply_palette(const QPalette& palette);
    void set_style(const QString& name);

    static QString color_to_string(const QColor& c);
    static QColor string_to_color(const QString& s);

    QMap<QString, QPalette> palettes;
    QString selected;
    QPalette default_palette;
    QString style;
};

} // namespace app::settings
