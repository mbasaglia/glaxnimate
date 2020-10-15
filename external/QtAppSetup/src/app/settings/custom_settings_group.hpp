#pragma once

#include <memory>
#include <QSettings>
#include <QIcon>

namespace app::settings {

class CustomSettingsGroupBase
{
public:
    virtual ~CustomSettingsGroupBase() = default;
    virtual QString slug() const = 0;
    virtual QString label() const = 0;
    virtual QIcon icon() const = 0;
    virtual void load(QSettings& settings) = 0;
    virtual void save(QSettings& settings) = 0;
    virtual QWidget* make_widget(QWidget* parent) = 0;
};


using CustomSettingsGroup = std::unique_ptr<CustomSettingsGroupBase>;

} // namespace app::settings
